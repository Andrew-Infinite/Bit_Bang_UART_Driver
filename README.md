# Kernel Module Project

This project contains a kernel module named `main.ko`.

## Building

To build the kernel module, run:

On Raspberry Pi:
```bash
sudo apt-get update
sudo apt-get install build-essential raspberrypi-kernel-headers


make
sudo insmod main.ko TX_PIN=YOUR_GPIO_NUMBER
sudo chmod 666 /dev/BitBangDevice
echo "This is test" > /dev/BitBangDevice
sudo rmmod main
```

On PC with UART TTL:
```
Use minicom or any serial analyzer to connect to your serial com
```
