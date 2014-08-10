## 依赖 ArchLinux

```
sudo pacman -S vtk
```

## 编译 

```
make clean
make
```

## 调试 

```
gdb ./vtkplot 
r ./sample/2s_zoom_in_sample.txt
```

## 使用 

```
cat /proc/bus/input/devices 知道了触摸板的设备文件描述符路径，例如event13
sudo ./read_mtdev /dev/input/event13
./vtkplot sample.txt
./vtkplot ./sample/2s_zoom_in_sample.txt 
./testgr sample.txt
./testgr ./sample/2s_zoom_in_sample.txt
```

## 测试用例 

testgr.cpp -> gesture_recognition
./sample 下是多点触摸采样数据 

