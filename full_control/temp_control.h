// RGB
#define Max_LED 3
#define MAX_SIZE 32
//
#define CPU_USAGE_PATH "/proc/stat"
#define HARDWARE_PATH "/proc/device-tree/model"
#define TEMP_PATH "/sys/class/thermal/thermal_zone0/temp"
// FAN SPEED
#define FAN_SPEED_FULL 0x01
#define FAN_SPEED_CLOSE 0x00
#define FAN_SPEED_PCT_20 0x02
#define FAN_SPEED_PCT_30 0x03
#define FAN_SPEED_PCT_40 0x04
#define FAN_SPEED_PCT_50 0x05
#define FAN_SPEED_PCT_60 0x06
#define FAN_SPEED_PCT_70 0x07
#define FAN_SPEED_PCT_80 0x08
#define FAN_SPEED_PCT_90 0x09
#define CONTRORL_DEVICE_RGB 0x07
#define CONTRORL_DEVICE_FAN 0x08

int fd_i2c;
void setRGB(int num, int R, int G, int B);
void closeRGB();
int setFan(int speed);
int control_fan(double temperature);
enum fan_speed {CLOSE=0x00,FULL,PCT_10,PCT_20,PCT_30,PCT_40,PCT_50,PCT_60,PCT_70,PCT_80,PCT_90};