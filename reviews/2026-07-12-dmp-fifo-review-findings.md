# Review Findings: DMP FIFO Investigation

Date: 2026-07-12  
Reviewed document: `issues/2026-07-04-dmp-fifo-review.md`

## Summary

The original review is a strong debugging handoff. Its register-first diagnostic strategy is the correct next step, and the three earlier fixes are legitimate architectural corrections. A few conclusions should be qualified, however, and the staged diagnostic can be made more decisive.

No source changes were made as part of this review.

## Findings

### 1. High: the battery conclusion is stronger than the available evidence

The software confirms that low battery telemetry does not intentionally disable sensor polling. `AppControl::update()` calls `sensor_.poll()` before applying the battery and angle motor-safety gate.

That establishes software independence, but it does not establish the board's electrical power topology. Without a schematic or a voltage measurement at the MPU6050 rail, the following claims are not conclusive:

- USB necessarily powers the complete 3.3 V sensor rail.
- Battery sag cannot disturb a shared regulator, ground, or enable circuit.
- Successful initialization proves continued power integrity after initialization.

A successful DMP upload is good evidence that gross power failure was absent during initialization. It does not exclude intermittent power, ground noise, or later rail sag. The battery is unlikely to be the primary cause, but the defensible conclusion is "software-independent and currently unlikely," rather than structurally impossible.

### 2. Medium: `IIC_MPU6050_Init()` is a confirmed porting omission, but not proven to be the root cause

The original firmware explicitly calls `IIC_MPU6050_Init()` before `MPU6050_initialize()`. The rewrite's `Imu::init()` currently begins with `MPU6050_initialize()` and contains no corresponding GPIO initialization call.

This is a real parity defect. Restoring the initialization is sensible, but calling it "zero-risk" is too strong for embedded GPIO configuration. The implementation also configures PB10 and PB11 as push-pull outputs rather than open-drain outputs. That matches the vendor code but is electrically questionable for I2C and should be documented.

There is also an unresolved contradiction: if PB11 remained in its reset-state input configuration, successful software-I2C traffic would be difficult to explain. Possibilities include prior GPIO configuration, debugger/reset behavior, observations from a different binary, or success being overstated because the I2C error path is permissive.

### 3. Medium: the register dump should print I2C return codes

The staged diagnostic initializes its register buffers to zero and ignores every `i2cRead()` result. Therefore this output:

```text
UC=00 PWR=00 INT=00 cnt=0
```

could mean either that zero values were genuinely read or that the reads failed and the initialized buffers remained unchanged.

The dump should record the return status for every read. Including a repeated `WHO_AM_I` read would also provide an immediate bus sanity check.

Reading `INT_STATUS` may clear latched status bits. That is probably harmless while no interrupt consumer exists, but the diagnostic changes the state it observes and should be treated accordingly.

### 4. Medium: `R=-1` alone does not prove the FIFO engine is broken

In this driver, `mpu_read_fifo_stream()` returns `-1` whenever the FIFO contains less than one complete DMP packet. With the currently enabled feature set, the packet length reaches 32 bytes.

The firmware loop runs at approximately the same 5 ms period as the configured 200 Hz DMP output. Depending on phase and UART/control-loop execution time, occasional calls before a packet is complete are normal.

The useful distinction is:

- Occasional `R=-1` with a varying FIFO count can mean "packet not ready yet."
- Persistent `R=-1` with a confirmed, successfully read `cnt=0` means the FIFO is not producing data.

The return code should therefore be interpreted together with validated FIFO count and control-register reads.

### 5. Low: the staged diagnostic contains a duplicated condition

`Imu::read()` currently contains two consecutive identical conditions:

```cpp
if (sensors & INV_WXYZ_QUAT)

if (sensors & INV_WXYZ_QUAT)
```

This behaves as two nested identical conditions and is unlikely to cause the frozen angle. It is nevertheless an editing artifact that should be removed before the final firmware is retained.

### 6. Low: the parity review omits the original startup delay

The reference startup waits approximately 300 ms before initializing software I2C and the MPU6050. The current firmware initializes the UART and proceeds directly to `sensor_hal.init()`.

Successful device identification and firmware upload make this an unlikely explanation for the persistent empty FIFO, but it belongs in a strict reference-sequence comparison.

## Confirmed strengths in the original review

The earlier fixes are sound:

1. Sensor acquisition is now explicitly driven through `ISensorHal::poll()`.
2. Quaternion and angle state have deterministic initial values.
3. IMU initialization failure propagates through `Stm32SensorHal::init()` to `main()`.

The `i2cWrite()` NACK finding is also correct. A NACK while writing a data byte currently returns success, and ACK results are ignored in other parts of the transaction. This weakens the inference that a successful high-level driver call proves that every low-level write landed correctly.

## Recommended diagnostic order

1. Restore explicit software-I2C GPIO initialization to match the reference startup.
2. Print I2C return codes alongside `WHO_AM_I`, `USER_CTRL`, `PWR_MGMT_1`, `FIFO_COUNT`, `INT_ENABLE`, and `INT_STATUS`.
3. Interpret `dmp_read_fifo()` only together with the validated FIFO count.
4. If the bus and DMP registers are valid, verify sample production and packet timing rather than continuing to inspect the already-matched initialization sequence.
5. Remove temporary diagnostics and the duplicated condition after the fault is isolated.

## Overall project assessment

The project's dependency direction is strong:

```text
application and PID logic
        -> HAL interfaces
        -> STM32 or simulation HAL
        -> BSP and vendor drivers
```

This keeps control logic host-testable and hardware knowledge near the edge. The simulation target, firmware target, retained reference tree, issue logs, and learning documentation make the repository useful both as working software and as an engineering record.

The main technical weakness is the boundary around the legacy C sensor driver:

- I2C errors are inconsistently reported.
- Some return values are ignored.
- Sensor freshness is not exposed to the application.
- A stale angle is indistinguishable from a valid constant angle.
- Hardware initialization depends on implicit vendor behavior rather than explicit checked invariants.

After resolving the FIFO issue, sensor health should become a first-class interface concept. A poll result could distinguish a new sample, no complete packet yet, bus error, FIFO overflow, and invalid quaternion. Motor safety could then depend on sample freshness as well as angle and battery thresholds. For a balancing robot, stale but plausible sensor data is a particularly important failure mode.

## Conclusion

The project direction is good and the original DMP review is mostly sound. Its strongest contribution is moving the investigation from static speculation to runtime register evidence. Before relying on the diagnostic, add explicit I2C status reporting, distinguish normal packet-not-ready results from a genuinely empty FIFO, and soften claims that depend on unverified board power topology.
