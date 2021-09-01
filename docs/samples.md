# fdt 示例程序 #

## fdt_dump

## 运行示例 ##
```sh
fdt_dump <dtb-filename>
```
## 示例结果 ##
实例结果应该为该设备树dts文本内容

## fdt_test

## 运行示例 ##
```sh
fdt_test
```
## 示例结果 ##
```sh
name = uart@9000
reg = <0x9000,0x1000>;
compatible = "arm,pl011","arm,primecell";

name = cpus
path = /cpus/cpu@0/
path = /cpus/cpu@1/
path = /cpus/cpu@2/
path = /cpus/cpu@3/

name = user1, lable = v2m:green:user1
name = user2, lable = v2m:green:user2
name = user3, lable = v2m:green:user3
name = user4, lable = v2m:green:user4
name = user5, lable = v2m:green:user5
name = user6, lable = v2m:green:user6
name = user7, lable = v2m:green:user7
name = user8, lable = v2m:green:user8

/memreserve/    0x0000000000000000 0x0000000000001000;

phandle = <0x9>
name = bt_pins
path = /soc/gpio@7e200000/bt_pins/
brcm,pins = [2d 00]
```