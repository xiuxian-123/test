#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

/**
 * @brief 初始化并打开 UART 串口
 * @param port 串口设备路径，如 "/dev/ttyUSB0"
 * @param baud 波特率，如 B9600, B115200
 * @return 成功返回文件描述符，失败返回 -1
 */

int uart_open(const char *port, speed_t baud) {
	int fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
	
	if (fd == -1) {
		
		perror("uart_open:open uart falied\n");
		return -1;	
	}

	fcntl(fd, F_SETFL, 0);

	struct termios tty;
	memset(&tty, 0, sizeof(tty));

	int rec = tcgetattr(fd, &tty);
	if (rec != 0) {
		
		perror("uart_open:tcgettattr failed");
		close(fd);
		return -1;
	
	}

	cfsetispeed(&tty, baud);
	cfsetospeed(&tty, baud);

	tty.c_cflag &= ~PARENB;
	tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;
	tty.c_cflag &= ~CRTSCTS;
	tty.c_cflag |= CREAD | CLOCAL;

	tty.c_iflag &= ~(IXON | IXOFF | IXANY);
	
	tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	tty.c_oflag &= ~OPOST;

	tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 10;

	rec = tcsetattr(fd, TCSANOW, &tty);
	if (rec != 0) {
	
		perror("uart_open:tcsetattr falied");
		close(fd);
		return -1;

	}

	tcflush(fd, TCIOFLUSH);
	return fd;

}

int main(void) {

	int fd = uart_open("/dev/ttyUSB0", B115200);
	if (fd < 0) {
		
		return -1;
	}

	char *msg = "Send to my heart";
	ssize_t len = write(fd, msg, strlen(msg));
	fprintf(stderr, "send %zd bit\n", len);

	char buf[256] = {0};
	len = read(fd, buf, sizeof(buf) - 1);
	if (len > 0) {
	
		fprintf(stderr, "接收到 %zd bit: %s\n", len, buf);
	} else if (len == 0) {
	
		fprintf(stderr, "读取超时，未收到数据\n");
	} else {
	
		perror("读取失败");
	}

	close(fd);
	return 0;

}
