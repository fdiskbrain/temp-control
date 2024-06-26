// 导入wiringPi/I2C库
#include <wiringPi.h>
#include <wiringPiI2C.h>

// 导入oled显示屏库
#include "ssd1306_i2c.h"

// 导入文件控制函数库
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
// 读取IP库
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
// 读取磁盘库
#include <sys/vfs.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <temp_control.h>
#include <unistd.h>
// 退出处理

void onexit(int sig)
{
	wiringPiSetup();
	fd_i2c = wiringPiI2CSetup(0x0d);
	ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);
	//   ssd1306_stopscroll();
	//    ssd1306_display();
	//    delay(5000);

	ssd1306_clearDisplay();
	ssd1306_display();
	wiringPiI2CWriteReg8(fd_i2c, 0x08, 0x00);
	setFan(FAN_SPEED_FULL);
	syslog(LOG_INFO, "clean and exit ");
	delay(2000);
	exit(EXIT_SUCCESS);
}
void reload(int sig)
{
	ssd1306_clearDisplay();
	printf("%s", "reload ok");
}

int main(void)
{
	printf("Starting\n");
	syslog(LOG_INFO, "starting fan controller");
	if (daemon(0, 0) != 0)
	{
		exit(EXIT_FAILURE);
	}
	int readed_ip = 0;
	int count = 0;
	int rgbclose = 1; // 1 close rgb,0 open rgb
	int fd_temp;
	int fd_hardware;	   // 保存查询到的树莓派硬件版本信息
	int raspi_version = 0; // 4:树莓派4B， 3:树莓派3B/3B+  0：无法识别或其他版本
	double temp = 0, last_temp = 0;
	int fan_state = 99; // init state 99
	char buf[MAX_SIZE];

	char buf_cpu[128];
	FILE *fp_CPUusage;
	char cpu[5];
	float usage;
	long int user, nice, sys, idle, iowait, irq, softirq;
	long int total_1, total_2, idle_1, idle_2;

	// get system usage / info
	struct sysinfo sys_info;
	struct statfs disk_info;

	struct ifaddrs *ifAddrStruct = NULL;
	void *tmpAddrPtr = NULL;

	getifaddrs(&ifAddrStruct);

	ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);

	// 定义I2C相关参数

	wiringPiSetup();
	fd_i2c = wiringPiI2CSetup(0x0d);
	signal(SIGTERM, onexit);
	signal(SIGINT, onexit);

	while (fd_i2c < 0)
	{
		fd_i2c = wiringPiI2CSetup(0x0d);
		fprintf(stderr, "fail to init I2C\n");
		delay(500);
	}

	// 开启RGB灯特效
	// wiringPiI2CWriteReg8(fd_i2c, 0x04, 0x03);

	// 关闭RGB灯特效
	wiringPiI2CWriteReg8(fd_i2c, 0x07, 0x00);
	wiringPiI2CWriteReg8(fd_i2c, 0x08, 0x00); // close fan
	// ssd1306_display(); //show logo
	ssd1306_clearDisplay();
	// delay(500);
	syslog(LOG_INFO, "Init ok");
	printf("Init ok, start loop\n");
	while (1)
	{
		// 读取系统信息
		// delay(900);
		if (sysinfo(&sys_info) != 0) // sysinfo(&sys_info) != 0
		{
			printf("sysinfo-Error\n");
			ssd1306_clearDisplay();
			char *text = "sysinfo-Error";
			ssd1306_drawString(text);
			ssd1306_display();
			continue;
		}
		else
		{
			// 清除屏幕内容
			ssd1306_clearDisplay();
			time_t curenttime;
			time(&curenttime);
			char Now[MAX_SIZE];
			sprintf(Now, "%s", ctime(&curenttime));
			// CPU占用率
			char CPUInfo[MAX_SIZE] = {0};
			// unsigned long avgCpuLoad = sys_info.loads[0] / 1000;
			// sprintf(CPUInfo, "CPU:%ld%%", avgCpuLoad);
			fp_CPUusage = fopen(CPU_USAGE_PATH, "r");
			if (fp_CPUusage == NULL)
			{
				printf("failed to open /proc/stat\n");
			}
			else
			{
				/* CPU使用率计算方式：usage = 100*(total-idle/total)
				 * 通过两次获取/proc/stat的数据差得到total和idle
				 */
				// 第一次读取数据
				fgets(buf_cpu, sizeof(buf_cpu), fp_CPUusage);
				sscanf(buf_cpu, "%s%ld%ld%ld%ld%ld%ld%ld", cpu, &user, &nice, &sys, &idle, &iowait, &irq, &softirq);
				total_1 = user + nice + sys + idle + iowait + irq + softirq;
				idle_1 = idle;
				rewind(fp_CPUusage);

				// 延时并且清空数据
				sleep(1);
				memset(buf_cpu, 0, sizeof(buf_cpu));
				cpu[0] = '\0';
				user = nice = sys = idle = iowait = softirq = 0;

				// 第二次读取数据
				fgets(buf_cpu, sizeof(buf_cpu), fp_CPUusage);
				sscanf(buf_cpu, "%s%ld%ld%ld%ld%ld%ld%ld", cpu, &user, &nice, &sys, &idle, &iowait, &irq, &softirq);
				total_2 = user + nice + sys + idle + iowait + irq + softirq;
				idle_2 = idle;

				usage = (float)(total_2 - total_1 - (idle_2 - idle_1)) / (total_2 - total_1) * 100;
				sprintf(CPUInfo, "CPU:%.0f%%", usage);
				// printf("cpu:%.0f%%\n", usage);
				fclose(fp_CPUusage);
			}

			// 运行内存剩余量，剩余/总内存
			char RamInfo[MAX_SIZE];
			unsigned long totalRam = sys_info.totalram >> 18;
			unsigned long freeRam = sys_info.freeram >> 18;
			// unsigned long totalRam = sys_info.totalram >> 20;
			// unsigned long freeRam = sys_info.freeram >> 20;
			// sprintf(RamInfo, "RAM:%ld/%ld GB", freeRam, totalRam);
			sprintf(RamInfo, "FM:%.0f%% Fan: %d", (float)freeRam / totalRam * 100, fan_state);
			// 获取IP地址
			char IPInfo[MAX_SIZE];
			if (readed_ip == 0)
			{
				while (ifAddrStruct != NULL)
				{
					if (ifAddrStruct->ifa_addr->sa_family == AF_INET)
					{ // check it is IP4 is a valid IP4 Address
						tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
						char addressBuffer[INET_ADDRSTRLEN];
						inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

						if (strcmp(ifAddrStruct->ifa_name, "eth0") == 0)
						{
							sprintf(IPInfo, "Eth0:%s", addressBuffer);
							readed_ip = 1;
							break;
						}
						else if (strcmp(ifAddrStruct->ifa_name, "wlan0") == 0)
						{
							sprintf(IPInfo, "wlan0:%s", addressBuffer);
							readed_ip = 1;
							break;
						}
						else
						{
							readed_ip = 0;
							sprintf(IPInfo, "NoIP:%s", "0");
						}
					}
					ifAddrStruct = ifAddrStruct->ifa_next;
				}
				getifaddrs(&ifAddrStruct);
			}

			// 读取树莓派硬件版本
			if (raspi_version == 0)
			{
				char hardware[MAX_SIZE];
				fd_hardware = open(HARDWARE_PATH, O_RDONLY);
				if (fd_hardware < 0)
				{
					printf("failed to open /proc/device-tree/model\n");
				}
				else
				{
					if (read(fd_hardware, hardware, MAX_SIZE) < 0)
					{
						printf("fail to read hardware version\n");
					}
					else
					{
						// 如果树莓派4B主板时，hardware=Raspberry Pi 4 Model B Rev 1.1
						char raspi[23] = "0";
						strncpy(raspi, hardware, 22);
						if (strcmp(raspi, "Raspberry Pi 4 Model B") == 0)
						{
							raspi_version = 4;
						}
						else if (strcmp(raspi, "Raspberry Pi 3 Model B") == 0)
						{
							raspi_version = 3;
						}
					}
				}
				close(fd_hardware);
			}

			// 读取CPU温度
			char CPUTemp[MAX_SIZE];
			fd_temp = open(TEMP_PATH, O_RDONLY);
			if (fd_temp < 0)
			{
				temp = 0;
				printf("failed to open thermal_zone0/temp\n");
			}
			else
			{
				// 读取温度并判断
				if (read(fd_temp, buf, MAX_SIZE) < 0)
				{
					temp = 0;
					printf("fail to read temp\n");
				}
				else
				{
					// 转换为浮点数打印
					temp = atoi(buf) / 1000.0;
					// printf("temp: %.1f\n", temp);
					sprintf(CPUTemp, "Temp:%.1fC", temp);
				}
			}
			close(fd_temp);

			// 读取磁盘空间，剩余/总空间
			char DiskInfo[MAX_SIZE];
			statfs("/", &disk_info);
			unsigned long long totalBlocks = disk_info.f_bsize;
			unsigned long long totalSize = totalBlocks * disk_info.f_blocks;
			size_t mbTotalsize = totalSize >> 30;
			unsigned long long freeDisk = disk_info.f_bfree * totalBlocks;
			size_t mbFreedisk = freeDisk >> 30;
			// sprintf(DiskInfo, "Disk:%ld/%ldGB", mbFreedisk, mbTotalsize);
			sprintf(DiskInfo, "FreeDisk:%.0f%%", (float)mbFreedisk / mbTotalsize * 100);

			// 在显示屏上要显示的内容
			ssd1306_drawText(0, 0, CPUInfo);
			ssd1306_drawText(56, 0, CPUTemp);
			ssd1306_drawText(0, 8, RamInfo);
			// ssd1306_drawText(0, 8, Now);
			ssd1306_drawText(0, 16, DiskInfo);
			ssd1306_drawText(0, 24, IPInfo);

			// 刷新显示
			ssd1306_display();
			// ssd1306_startscrollright(0x00, 0x0F);
			// delay(4060);
			// ssd1306_stopscroll();
			// delay(500);
			ssd1306_startscrollleft(0x00, 0x0F);
			delay(4060);
			ssd1306_stopscroll();
		}
		ssd1306_clearDisplay();
		//        printf("pi version %d!\n", raspi_version);

		// fan control
		if (raspi_version == 4)
		{
			if (abs(temp - last_temp) > 3.0) // 检查温度偏差小于4度不做改变状态
			{
				last_temp = temp;
				fan_state = setFan(get_speed(temp));
			}
			syslog(LOG_INFO | LOG_USER, "pi %d; fan: %d;temperature: %.1f;last temperature: %.1f\n", raspi_version, fan_state, temp, last_temp);
		}
		else if (raspi_version == 3)
		{
			if (temp >= 46)
			{
				if (fan_state != 1)
				{
					wiringPiI2CWriteReg8(fd_i2c, 0x08, 0x01);
					fan_state = 1;
				}
			}
			else if (temp <= 40)
			{
				if (fan_state != 0)
				{
					wiringPiI2CWriteReg8(fd_i2c, 0x08, 0x00);
					fan_state = 0;
				}
			}
		}
		else // if (raspi_version == 0)
		{
			if (temp >= 55)
			{
				if (fan_state != 1)
				{
					wiringPiI2CWriteReg8(fd_i2c, 0x08, 0x01);
					fan_state = 1;
				}
			}
			else if (temp <= 48)
			{
				if (fan_state != 0)
				{
					wiringPiI2CWriteReg8(fd_i2c, 0x08, 0x00);
					fan_state = 0;
				}
			}
		}
		// delay(500);
		//  RBG
		if (rgbclose == 0)
		{
			if (count == 10)
			{
				// 开启RGB灯特效
				wiringPiI2CWriteReg8(fd_i2c, 0x04, 0x04);
			}
			else if (count == 20)
			{
				// 开启RGB灯特效
				wiringPiI2CWriteReg8(fd_i2c, 0x04, 0x02);
			}
			else if (count == 30)
			{
				// 开启RGB灯特效
				wiringPiI2CWriteReg8(fd_i2c, 0x04, 0x01);
			}
			else if (count == 40)
			{
				// 开启RGB灯特效
				wiringPiI2CWriteReg8(fd_i2c, 0x04, 0x03);
				count = 0;
			}
			else if (count == 0)
			{
				wiringPiI2CWriteReg8(fd_i2c, 0x04, 0x00);
			}

			count++;
			// delay(500);
		}

		// else
		// {
		// 	// 关RGB灯特效
		// 	wiringPiI2CWriteReg8(fd_i2c, 0x07, 0x00);
		// }

		// rgb temp
		if (abs(temp - last_temp) >= 1)
		{
			if (temp <= 45)
			{
				setRGB(Max_LED, 0x00, 0x00, 0xff);
			}
			else if (temp <= 47)
			{
				setRGB(Max_LED, 0x1e, 0x90, 0xff);
			}
			else if (temp <= 49)
			{
				setRGB(Max_LED, 0x00, 0xbf, 0xff);
			}
			else if (temp <= 51)
			{
				setRGB(Max_LED, 0x5f, 0x9e, 0xa0);
			}

			else if (temp <= 53)
			{
				setRGB(Max_LED, 0xff, 0xff, 0x00);
			}
			else if (temp <= 55)
			{
				setRGB(Max_LED, 0xff, 0xd7, 0x00);
			}
			else if (temp <= 57)
			{
				setRGB(Max_LED, 0xff, 0xa5, 0x00);
			}
			else if (temp <= 59)
			{
				setRGB(Max_LED, 0xff, 0x8c, 0x00);
			}

			else if (temp <= 61)
			{
				setRGB(Max_LED, 0xff, 0x45, 0x00);
			}
			else if (temp >= 63)
			{
				setRGB(Max_LED, 0xff, 0x00, 0x00);
			}
		}
		delay(500);
	}
	// atexit(onexit);
	return 0;
}
// 设置RGB灯,num如果大于等于Max_LED（3），则全部灯一起设置
// num=(0 ~ 3),R=(0 ~ 255),G=(0 ~ 255),B=(0 ~ 255)
void setRGB(int num, int R, int G, int B)
{
	if (num >= Max_LED)
	{
		wiringPiI2CWriteReg8(fd_i2c, 0x00, 0xff);
		wiringPiI2CWriteReg8(fd_i2c, 0x01, R);
		wiringPiI2CWriteReg8(fd_i2c, 0x02, G);
		wiringPiI2CWriteReg8(fd_i2c, 0x03, B);
	}
	else if (num >= 0)
	{
		wiringPiI2CWriteReg8(fd_i2c, 0x00, num);
		wiringPiI2CWriteReg8(fd_i2c, 0x01, R);
		wiringPiI2CWriteReg8(fd_i2c, 0x02, G);
		wiringPiI2CWriteReg8(fd_i2c, 0x03, B);
	}
}
int setFan(int speed)
{
	wiringPiI2CWriteReg8(fd_i2c, CONTRORL_DEVICE_FAN, speed);
	syslog(LOG_INFO, "Setting fan speed %d", speed);
	return speed;
}
// 关闭RGB
void closeRGB()
{
	wiringPiI2CWriteReg8(fd_i2c, CONTRORL_DEVICE_RGB, 0x00);
	delay(10);
}
// speed 0: close,1: full, 2 - 9 pct
int get_speed(double temperature)
{
	int level = 0;
	level = (int)temperature / 5;
	syslog(LOG_INFO, "Trace temperature : %.1f;level: %d", temperature, level);
	if (level <= 5)
	{
		return 0;
	}
	if (level >= 10)
	{
		return 1;
	}
	return level;
}
