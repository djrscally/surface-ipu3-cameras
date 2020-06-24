#### prerequisite patches

Apply software-node patches to your kernel first, from jhand2's this dir:
- https://github.com/jhand2/surface-camera/tree/master/patches

#### build

```bash
make
```

#### usage

```bash
# first, load your camera sensor driver, then
sudo insmod surface_camera.ko
```

#### SB1_ov5693

Contrary to the prefixed name `SB1_`, the driver in the dir may work for
all the Surface devices, provided that the port num the ov5693 sensor
connected to is the same as SB1 (`port1`).