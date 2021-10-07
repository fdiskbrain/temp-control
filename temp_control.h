// RGB
#define MAX_SIZE 20
#define Max_LED 3
//
#define CPU_USAGE_PATH "/proc/stat"
#define HARDWARE_PATH "/proc/device-tree/model"
#define TEMP_PATH "/sys/class/thermal/thermal_zone0/temp"
#define MAX_SIZE 32

int fd_i2c;
void setRGB(int num, int R, int G, int B);
void closeRGB();