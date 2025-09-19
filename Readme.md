
## How to Use

### Linux or Mac

```shell
cd tools
sh build_native_linux_release.sh
```

### Window

```shell
./tools/build_native_win_release.bat
```

## parse STP File to struct and mesh JSON
```sh
./OcctParser filepath {"linearUnit":"millimeter","linearDeflection":0.1,"angularDeflection":0.05}
```