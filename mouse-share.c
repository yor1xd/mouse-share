#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <libevdev/libevdev.h>
#include <linux/input.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <fcntl.h>
#include <libevdev-1.0/libevdev/libevdev-uinput.h>
#include <libevdev-1.0/libevdev/libevdev.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int setup(char *ip, char *port, int mode, int *descriptor) {

  struct addrinfo hints, *res;

  int sd, rc, cd;
  char *msg = "Hello";
  *descriptor = -1;

  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  rc = getaddrinfo(ip, port, &hints, &res);
  if (rc == -1) {
    printf("Could not get address info on %s:%s\n", ip, port);

    return -1;
  }

  sd = socket(res->ai_addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
  if (sd == -1) {
    printf("Could not get socket descriptor.\n");

    return -1;
  }

  if (mode == 1) {
    rc = bind(sd, res->ai_addr, res->ai_addrlen);
    if (rc == -1) {
      printf("Could not bind to address.\n");

      return -1;
    }

    rc = listen(sd, 5);
    if (rc == -1) {
      printf("Could not listen on address.\n");

      return -1;
    }

    cd = accept(sd, NULL, NULL);
    if (cd == -1) {
      printf("Could not accept incoming connection.\n");

      return -1;
    }

    *descriptor = cd;
  }

  if (mode == 0) {
    rc = connect(sd, res->ai_addr, res->ai_addrlen);
    if (rc == -1) {
      printf("Could not connect to %s", ip);

      return -1;
    }

    *descriptor = sd;
  }

  return 0;
}

int main(int argc, char *argv[]) {
  int mode, sd, rc;
  char *ip;
  char *port;

  printf("Do you want to: \n");
  printf("0 - Connect, 1 - Listen, Other - Exit\n");
  printf("Enter mode: ");
  scanf("%d", &mode);

  if (mode != 0 && mode != 1) {
    printf("Exiting...\n");

    return 1;
  }

  if (mode == 1) {
    printf("Listening...\n");

    struct libevdev *dev = NULL;
    int fd;
    int rc = -1;

    fd = open("/dev/input/event7", O_NONBLOCK | O_RDONLY);

    rc = libevdev_new_from_fd(fd, &dev);
    if (rc == -1) {
      printf("Could not create evdev struct.\n");

      return -1;
    }

    rc = setup("192.168.1.101", "4444", 1, &sd);
    if (rc == -1) {
      printf("Could not setup connection.\n");

      return -1;
    }

    // Loop
    for (int i = 0; i < 100; i++) {
      struct input_event event;
      libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &event);

      printf("Buffer size: %lu bytes.\n", sizeof(event));

      rc = send(sd, &event, sizeof(event), 0);
      printf("%d bytes were sent.\n", rc);

      usleep(500000);
    }
  }

  if (mode == 0) {
    printf("IPv4: ");
    scanf("%s", ip);

    printf("PORT: ");
    scanf("%s", port);

    rc = setup(ip, port, 0, &sd);
    if (rc == -1) {
      printf("Could not setup connection.\n");

      return -1;
    }

    for (int i = 0; i < 100; i++) {
      struct input_event event;

      rc = recv(sd, &event, sizeof(event), 0);
      if (rc == -1) {
        printf("Could not receive buffer.\n");
      }

      printf("Event: %s %s %d", libevdev_event_type_get_name(event.type),
             libevdev_event_code_get_name(event.type, event.code), event.value);
    }
  }

  return 0;
}
