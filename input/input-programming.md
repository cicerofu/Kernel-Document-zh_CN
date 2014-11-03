# 开发输入驱动

***Copyright (C) 2014 Leslie Zhai <xiang.zhai@i-soft.com.cn>***

> https://www.kernel.org/doc/Documentation/input/input-programming.txt


## 1. 开发输入设备驱动


### 1.0 简单的例子

这是一个输入设备驱动的简单例子（复杂的可以参考wacom的input-wacom驱动）。设备上
只有一个按钮，可以通过i/o端口BUTTON_PORT访问。当按下或松开，触发BUTTON_IRQ中断。
代码如下所示：

```
#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>

#include <asm/irq.h>
#include <asm/io.h>

static struct input_dev *button_dev;

static irqreturn_t button_interrupt(int irq, void *dummy)
{
	input_report_key(button_dev, BTN_0, inb(BUTTON_PORT) & 1);
	input_sync(button_dev);
	return IRQ_HANDLED;
}

static int __init button_init(void)
{
	int error;

	if (request_irq(BUTTON_IRQ, button_interrupt, 0, "button", NULL)) {
        printk(KERN_ERR "button.c: Can't allocate irq %d\n", button_irq);
        return -EBUSY;
    }

	button_dev = input_allocate_device();
	if (!button_dev) {
		printk(KERN_ERR "button.c: Not enough memory\n");
		error = -ENOMEM;
		goto err_free_irq;
	}

	button_dev->evbit[0] = BIT_MASK(EV_KEY);
	button_dev->keybit[BIT_WORD(BTN_0)] = BIT_MASK(BTN_0);

	error = input_register_device(button_dev);
	if (error) {
		printk(KERN_ERR "button.c: Failed to register device\n");
		goto err_free_dev;
	}

	return 0;

 err_free_dev:
	input_free_device(button_dev);
 err_free_irq:
	free_irq(BUTTON_IRQ, button_interrupt);
	return error;
}

static void __exit button_exit(void)
{
    input_unregister_device(button_dev);
	free_irq(BUTTON_IRQ, button_interrupt);
}

module_init(button_init);
module_exit(button_exit);
```


### 1.1 解释例子代码

首先包含<linux/input.h>头文件，输入子系统的接口。

内核模块被加载，或启动内核的时候，会调用_init函数，获取所需要的资源（它也会检查
该设备是否存在）。

使用input_allocate_device()分配一个新的输入设备结构体，并设置输入位域。设备驱动
向其他输入系统介绍自己——该输入设备可以产生或接收哪些事件。该例子中，设备只能从
BTN_0事件代码产生EV_KEY类型的事件。所以我们只用设置两位。

```
	set_bit(EV_KEY, button_dev.evbit);
	set_bit(BTN_0, button_dev.keybit);
```

然后例子驱动调用input_register_device注册输入设备结构体。

将button_dev结构体添加到输入驱动的链表，并调用设备处理模块的_connect函数，通知
大家一个新输入设备来了。input_register_device可能会sleep一下，所以不能在中断或
spinlock锁里调用。

驱动唯一用到的函数是button_interrupt()产生中断，按钮检查它的状态，通过
input_report_key向输入系统报告。

这里没有必要人为检查中断是否向输入系统报告了两次相同的事件（比如按下、按下），
因为input_report_*函数已经做了重复检查。

然后调用input_sync通知事件接收者我们已经发送了一个完整的报告。一个按钮的情况
凸显不了input_sync的重要性，但是当鼠标移动的时候，用户态肯定不想在独立的中断里
分别获取X和Y的值，这就需要input_sync通知X和Y的值我们都报告完毕了，比如wacom驱动
input-wacom，就需要input_sync通知X、Y、压感...的值都报告给用户态。


### 1.2 dev->open() 和 dev->close()

因为没有中断，驱动得不间断地调用poll来监听I/O事件，而poll是重操作，如果设备使用
中断，就可以使用open、close回调函数来判断何时可以停止poll，何时可以释放中断，
何时必须唤醒poll，恢复中断现场。

```
static int button_open(struct input_dev *dev)
{
	if (request_irq(BUTTON_IRQ, button_interrupt, 0, "button", NULL)) {
        printk(KERN_ERR "button.c: Can't allocate irq %d\n", button_irq);
        return -EBUSY;
    }

    return 0;
}

static void button_close(struct input_dev *dev)
{
    free_irq(IRQ_AMIGA_VERTB, button_interrupt);
}

static int __init button_init(void)
{
	...
	button_dev->open = button_open;
	button_dev->close = button_close;
	...
}
```

注意输入核心监视设备的用户数，确保只有当第一个用户连接到设备的时候才调用
dev->open()，当迅速断开连接的时候调用dev->close()。序列化open和close的回调函数。

open()的回调函数返回0代表正确，非0代表错误。close()回调函数（返回类型void）常真。


### 1.3 基本的事件类型

最简单的事件类型是EV_KEY用于键和按钮。通过调用

```
	input_report_key(struct input_dev *dev, int code, int value)
```

报告给输入系统。

查看linux/input.h事件代码的取值范围[0, KEY_MAX]。非0表示键按下，0表示键释放。
仅当值变化的时候产生事件。

除了EV_KEY，还有其他两种基础事件类型：EV_REL和EV_ABS。它们是设备提供的“相对值”
和“绝对值”。相对值比如鼠标在X轴移动。鼠标报告的是（与上一次坐标做比较）相对的
变化值，因为它（在物理世界）没有任何绝对坐标系（只有相对坐标系）。绝对值对于
游戏手柄、比如wacom手写板——这些设备工作在绝对坐标系。

EV_REL和设备报告EV_KEY一样简单，设置相应的位，调用该函数

```
	input_report_rel(struct input_dev *dev, int code, int value)
```

事件只产生非0值。

但是EV_ABS需要特别注意一点。在调用input_register_device之前，需要对input_dev
结构体中针对设备的每个绝对坐标进行赋值。如果我们的按钮设备也有ABS_X坐标：

```
	button_dev.absmin[ABS_X] = 0;   // X轴最小值
	button_dev.absmax[ABS_X] = 255; // X轴最大值
	button_dev.absfuzz[ABS_X] = 4;  // 线性人为添加噪音幅值
	button_dev.absflat[ABS_X] = 8;  // 中心点的大小
```

或者直接调用：

```
	input_set_abs_params(button_dev, ABS_X, 0, 255, 4, 8);
```

该设置是针对游戏手柄的X轴，最小值0, 最大值255（游戏手柄必须能报告最大值，报告
成256都不要紧，但是它必须在[0, 255]范围内能报告），线性人为添加噪音幅值+-4，
中心点的大小8。

如果不需要absfuzz和absflat，可以把它们设置为0，意味着报告给用户态的数值不会被
+-4。


### BITS_TO_LONGS(), BIT_WORD(), BIT_MASK()

bitops.h里的这三个宏提供位域计算：

```
	BITS_TO_LONGS(x) - 返回位域数组的长度 
	BIT_WORD(x)	 - 返回某位在数组中的索引
	BIT_MASK(x)	 - 返回某位的索引
```

### id* 和 name 字段

在输入设备驱动注册输入设备之前需要设置dev->name值。比如字符串'Generic button device'
起一个友好的设备名。

id*字段包含bus ID (PCI、USB ...)，开发商ID和设备ID。input.h里定义了bus IDs。
pci_ids.h、usb_ids.h和相似的头文件里定义了开发商和设备ids。输入设备驱动注册它
之前必须设置之。

idtype字段用来表示输入设备驱动的特别信息。

通过evdev接口可以在用户态获得id和name字段。


### keycode、keycodemax、keycodesize字段

具有很多keymaps的输入设备需要使用这仨字段。keycode是个数组用来映射scancodes到
输入系统keycodes。keycode max需要包含该数组的长度，keycodesize表示每个元素的
（字节）大小。

用户态可以在相应的evdev接口上调用EVIOCGKEYCODE和EVIOCSKEYCODE的ioctls来搜索、
改变当前scancode到keycode的映射。当设备初始化了上述仨字段，驱动就会默认使用内核
的默认配置，搜索keycode映射。


### dev->getkeycode() and dev->setkeycode()

getkeycode() 和 setkeycode() 回调函数允许驱动重载默认的
keycode/keycodesize/keycodemax 映射机制。


### 键重复输入

简单。由input.c模块处理。不会用到硬件重复输入，因为不是所有的硬件设备都支持，
即便支持的，耗材很容易老损。在dev->evbit中设置EV_REP，就可以启用设备的重复输入。
全部由输入系统处理。


### 其他事件类型，处理输出事件

```
EV_LED - 键盘LED灯
EV_SND - 键盘蜂鸣
```

它们和键盘事件类似，但是相反的方向——从系统到输入设备驱动。如果你的输入设备驱动
可以处理这些事件，需要在evit中设置相应的位、回调函数：

```
	button_dev->event = button_event;

int button_event(struct input_dev *dev, 
                 unsigned int type, 
                 unsigned int code, 
                 int value);
{
	if (type == EV_SND && code == SND_BELL) {
		outb(value, BUTTON_BELL);
		return 0;
	}
	return -1;
}
```

可以在中断、BH里调用回调函数，不需要sleep，轻操作而已。
