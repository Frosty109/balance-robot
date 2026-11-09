# One HC-SR04 result from an STM32 RANGE line; header.stamp is host receipt time.
std_msgs/Header header

uint8 STATUS_STARTING=0
uint8 STATUS_VALID=1
uint8 STATUS_NO_ECHO=2
uint8 STATUS_TIMEOUT=3
uint8 STATUS_OUT_OF_RANGE=4
uint8 STATUS_CAPTURE_ERROR=5
uint8 status

uint32 sequence
uint32 completed_ms
# Only meaningful when status is STATUS_VALID.
uint32 distance_mm
uint32 reported_age_ms
