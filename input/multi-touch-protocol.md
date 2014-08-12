# 多点触摸协议

> https://www.kernel.org/doc/Documentation/input/multi-touch-protocol.txt


## 介绍

要想使用强大的多点触摸设备，就需要一种可以报告多点触摸的详细数据的方法，例如，
直接触摸设备表面的对象。这篇文档描述多点触摸（以下简称MT）协议，让内核驱动报告
多点触摸的详细信息。

根据设备功能，协议分为两类。匿名触摸（A类），向接收器发送所有触摸点的raw数据。
可跟踪id触摸（B类），通过事件槽向每个触摸点发送更新数据。


## 协议的使用

触摸点的详细数据就是顺序发送独立的ABS_MT事件数据包。触摸数据包中只有ABS_MT事件
才能被识别。虽然单点触摸（ST）应用程序忽略多点触摸事件，但是依然可以在现有的
单点触摸协议驱动上实现多点触摸协议。

A类驱动，在数据包结尾处调用input_mt_sync()区分其他的触摸数据包。产生一个
SYN_MT_REPORT事件，通知接收器接收当前触摸的数据，并准备接收下一个数据。

B类驱动，在数据包开头处通过调用input_mt_slot()，槽作为参数，区分其他的触摸数据
包。产生一个ABS_MT_SLOT事件，通知接收器准备更新当前槽。

A、B类驱动，都是调用input_sync()标识多点触摸会话的结束。通知接收器执行
EV_SYN/SYN_REPORT之前的所有事件，并准备接收新一轮的信号/数据包。

无状态的A类协议和有状态的B类槽协议的主要区别是使用触摸id，减少了向用户态发送的
数据量。槽协议需要使用ABS_MT_TRACKING_ID，由硬件提供，或从raw数据中获取[5]。

在A类设备表面触摸，内核驱动会产生无序的当前匿名触摸数据。出现在事件流里的数据包
没有先后顺序。事件过滤、手指跟踪的特性功能留给用户态来实现[3]。

B类设备，内核驱动会将槽与每个可标识的触摸关联起来，使用槽描述与之相对应的触摸
产生的变化。通过修改ABS_MT_TRACKING_ID与之相对应的槽，实现触摸的构造、变换和
析构。一个非负数的跟踪id被当作为一次触摸，值-1表示该槽还未被使用。之前从未出现
过的跟踪id是新id，而不再出现的跟踪id则是被删除的。因为只描述变化量，所以每个刚
开始的触摸要在接收的结尾处才能知道其全部状态。当接收到一个MT事件，简单更新当前
槽的某个属性。

某些设备标识、跟踪更多的触摸，但是没有完全反馈给驱动。该设备驱动会将B类槽与硬件
报告的每个触摸关联起来。当与槽相关的触摸id变化了，驱动使该槽无效，更改它的
ABS_MT_TRACKING_ID。如果跟踪到的触摸总数大于当前报告的，驱动就会使用
BTN_TOOL_*TAP事件通知用户态由硬件跟踪到的触摸总数。驱动发送BTN_TOOL_*TAP事件，
并在调用input_mt_report_pointer_emulation()的时候，将use_count设置为false。硬件
支持槽的总数，驱动就通知之。用户态可以发现驱动可以报告比BTN_TOOL_*TAP事件支持的
最大值更多的触摸总数，多于B类槽报告的ABS_MT_SLOT的absinfo触摸总数。

ABS_MT_SLOT的最小值必须是0。

## A类协议的例子

A类设备的两个触摸点的最小事件序列如下所示：

```
   ABS_MT_POSITION_X x[0]
   ABS_MT_POSITION_Y y[0]
   SYN_MT_REPORT
   ABS_MT_POSITION_X x[1]
   ABS_MT_POSITION_Y y[1]
   SYN_MT_REPORT
   SYN_REPORT
```

每个触摸点的序列看上去完全一样；raw数据之间用同步SYN_REPORT分隔。

当第一个触摸点（移开第一根手指）离开时的序列：

```
   ABS_MT_POSITION_X x[1]
   ABS_MT_POSITION_Y y[1]
   SYN_MT_REPORT
   SYN_REPORT
```

这是第二个触摸点也离开时的序列：
```
   SYN_MT_REPORT
   SYN_REPORT
```

如果驱动还向ABS_MT事件中报告了BTN_TOUCH或ABS_PRESSURE，最后的SYN_MT_REPORT事件
将被忽略。否则，最后的SYN_REPORT事件将被遗漏，结果就变成没有触摸事件到达用户态。


## B类协议的例子

B类设备的两个触摸点的最小事件序列如下所示：

```
   ABS_MT_SLOT 0
   ABS_MT_TRACKING_ID 45
   ABS_MT_POSITION_X x[0]
   ABS_MT_POSITION_Y y[0]
   ABS_MT_SLOT 1
   ABS_MT_TRACKING_ID 46
   ABS_MT_POSITION_X x[1]
   ABS_MT_POSITION_Y y[1]
   SYN_REPORT
```

这是触摸ABS_MT_TRACKING_ID为45的触摸点在X轴移动的序列：

```
   ABS_MT_SLOT 0
   ABS_MT_POSITION_X x[0]
   SYN_REPORT
```

这是槽（序号）为0的触摸离开的序列：

```
   ABS_MT_TRACKING_ID -1
   SYN_REPORT
```

被更改的槽（序号）已经为0，所以ABS_MT_SLOT就被忽略了。删除和ABS_MT_TRACKING_ID
为45、槽（序号）为0的触摸信息，析构ABS_MT_TRACKING_ID为45的触摸，释放
（序号为0）槽，供其他的触摸来复用。

最后，第二个触摸点也离开时的序列：

```
   ABS_MT_SLOT 1
   ABS_MT_TRACKING_ID -1
   SYN_REPORT
```

## 事件的使用

定义了一组包含属性的ABS_MT事件。事件分为多种，允许只实现了其中某一部分。最小的
事件集包含ABS_MT_POSITION_X和ABS_MT_POSITION_Y，可以跟踪多点触摸。如果设备支持
ABS_MT_TOUCH_MAJOR和ABS_MT_WIDTH_MAJOR，就可以知道触摸区域的大小、支持的工具。

用几何来解释TOUCH和WIDTH参数；假设向一个窗户看过去，某人正轻轻地指向这面玻璃。
你将看到两个区域，一个由真实触摸在玻璃上的某一部分手指所构成的内部区域，和一个
由手指周长所构成的外部区域。触摸区域(a)的中心是ABS_MT_POSITION_X/Y，而手指(b)的
中心是ABS_MT_TOOL_X/Y。触摸的直径是ABS_MT_TOUCH_MAJOR，而手指的直径是
ABS_MT_WIDTH_MAJOR。现在假设某人使劲按玻璃。触摸区域将会变大，也就是，
ABS_MT_TOUCH_MAJOR / ABS_MT_WIDTH_MAJOR的比率，永远比单位一小，和触摸的压感有关。
支持压感的设备，ABS_MT_PRESSURE可以用来提供触摸范围内的压感值。可以使用
ABS_MT_DISTANCE表示触摸和表面的间距。


      Linux MT                               Win8
         __________                     _______________________
        /          \                   |                       |
       /            \                  |                       |
      /     ____     \                 |                       |
     /     /    \     \                |                       |
     \     \  a  \     \               |       a               |
      \     \____/      \              |                       |
       \                 \             |                       |
        \        b        \            |           b           |
         \                 \           |                       |
          \                 \          |                       |
           \                 \         |                       |
            \                /         |                       |
             \              /          |                       |
              \            /           |                       |
               \__________/            |_______________________|


除了MAJOR参数，可以用MINOR参数来描述触摸、手指的椭圆形状，MAJOR和MINOR是椭圆形
的长、短轴。可以用ORIENTATION参数来描述触摸的椭圆形的旋转，使用矢量减法
(a - b)知道椭圆形的方向。

A类设备，ABS_MT_BLOB_ID可以进一步描述触摸的形状。

ABS_MT_TOOL_TYPE可以区分是通过手指、笔还是其他工具触摸的。最后，
ABS_MT_TRACKING_ID事件可以用来实时地跟踪触摸id[5]。

B类协议，ABS_MT_TOOL_TYPE和ABS_MT_TRACKING_ID是由输入核心处理；驱动应该调用
input_mt_report_slot_state()。

## 事件概念

ABS_MT_TOUCH_MAJOR

触摸长轴的长度。长度和触摸表面的分辨率有关。如果分辨率是X * Y，
ABS_MT_TOUCH_MAJOR的对角线[4]的最大值是sqrt(X^2 + Y^2)。

ABS_MT_TOUCH_MINOR

以触摸表面为单位，短轴的长度。如果触摸的形状是圆形，就可以忽略[4]这个事件。

ABS_MT_WIDTH_MAJOR

以触摸表面为单位，工具的长轴长度。工具本身的大小。触摸和工具的旋转是一样的[4]。

ABS_MT_WIDTH_MINOR

以触摸表面为单位，工具的短轴长度。忽略圆形。

上述的四个值可以获得触摸的附加信息。ABS_MT_TOUCH_MAJOR / ABS_MT_WIDTH_MAJOR的
比值大致等于压感。手指和手掌的宽度不同。

ABS_MT_PRESSURE

任意单位的触摸表面的压感。支持压感或支持空间信号强度分布的设备可以用
ABS_MT_PRESSURE替代TOUCH和WIDTH的比值。

ABS_MT_DISTANCE

以触摸表面为单位，触摸和触摸面的间距。间距为0表示正在触摸表面。正数表示触摸在
触摸面上悬空。

ABS_MT_ORIENTATION

触摸椭圆形的旋转。该值表示以触摸中心点为圆心，正时针旋转的刻度。值域范围是任意
的，以触摸表面Y轴正方向为0度，负数表示向左旋转，正数表示向右旋转。当围绕X轴旋转
一周，返回值域范围内的最大值。

触摸椭圆形默认是轴对称的。但是我觉得一般人的手最多旋转180度。

如果触摸面是圆形，旋转值就可以忽略不计，或者内核驱动不提供该信息。如果设备
可以区分长短轴，就可以部分支持旋转。ABS_MT_ORIENTATION的值域是[0, 1]。[4]

ABS_MT_POSITION_X

触摸椭圆的中心点在触摸表面的坐标X值。

ABS_MT_POSITION_Y

触摸椭圆的中心点在触摸表面的坐标Y值。

ABS_MT_TOOL_X

工具的中心点在触摸表面的坐标X值。如果设备无法区分触摸点和工具本身，忽略该值。

ABS_MT_TOOL_Y

工具的中心点在触摸表面的坐标Y值。如果设备无法区分触摸点和工具本身，忽略该值。

The four position values can be used to separate the position of the touch
from the position of the tool. If both positions are present, the major
tool axis points towards the touch point [1]. Otherwise, the tool axes are
aligned with the touch axes.
四种位置值可以用来区分触摸和工具的位置。

ABS_MT_TOOL_TYPE

工具的种类。大多数内核驱动无法区别不同工具的类型，比如无法区分是一根手指还是
一只笔。忽略该事件。协议目前支持MT_TOOL_FINGER和MT_TOOL_PEN[2]。B类设备，由输入
核心来处理该事件；驱动调用input_mt_report_slot_state()。

ABS_MT_BLOB_ID

BLOB_ID将多个数据包分组成一个任意形状的触摸。形成多边形的离散点序列定义了触摸的
形状。这是针对A类设备的底层匿名分组，不会和可跟踪ID[5]弄混淆。大多数A类设备不
支持blob，所以驱动可以忽略该事件。

ABS_MT_TRACKING_ID

TRACKING_ID识别一个触摸，并贯穿它的整个生命周期[5]。TRACKING_ID的值域要足够大，
保证在漫长的触摸体验时，触摸id是唯一的。B类设备，由输入核心管理该事件；驱动调用
input_mt_report_slot_state()。


Event Computation
-----------------

The flora of different hardware unavoidably leads to some devices fitting
better to the MT protocol than others. To simplify and unify the mapping,
this section gives recipes for how to compute certain events.

For devices reporting contacts as rectangular shapes, signed orientation
cannot be obtained. Assuming X and Y are the lengths of the sides of the
touching rectangle, here is a simple formula that retains the most
information possible:

   ABS_MT_TOUCH_MAJOR := max(X, Y)
   ABS_MT_TOUCH_MINOR := min(X, Y)
   ABS_MT_ORIENTATION := bool(X > Y)

The range of ABS_MT_ORIENTATION should be set to [0, 1], to indicate that
the device can distinguish between a finger along the Y axis (0) and a
finger along the X axis (1).

For win8 devices with both T and C coordinates, the position mapping is

   ABS_MT_POSITION_X := T_X
   ABS_MT_POSITION_Y := T_Y
   ABS_MT_TOOL_X := C_X
   ABS_MT_TOOL_X := C_Y

Unfortunately, there is not enough information to specify both the touching
ellipse and the tool ellipse, so one has to resort to approximations.  One
simple scheme, which is compatible with earlier usage, is:

   ABS_MT_TOUCH_MAJOR := min(X, Y)
   ABS_MT_TOUCH_MINOR := <not used>
   ABS_MT_ORIENTATION := <not used>
   ABS_MT_WIDTH_MAJOR := min(X, Y) + distance(T, C)
   ABS_MT_WIDTH_MINOR := min(X, Y)

Rationale: We have no information about the orientation of the touching
ellipse, so approximate it with an inscribed circle instead. The tool
ellipse should align with the vector (T - C), so the diameter must
increase with distance(T, C). Finally, assume that the touch diameter is
equal to the tool thickness, and we arrive at the formulas above.

Finger Tracking
---------------

The process of finger tracking, i.e., to assign a unique trackingID to each
initiated contact on the surface, is a Euclidian Bipartite Matching
problem.  At each event synchronization, the set of actual contacts is
matched to the set of contacts from the previous synchronization. A full
implementation can be found in [3].


Gestures
--------

In the specific application of creating gesture events, the TOUCH and WIDTH
parameters can be used to, e.g., approximate finger pressure or distinguish
between index finger and thumb. With the addition of the MINOR parameters,
one can also distinguish between a sweeping finger and a pointing finger,
and with ORIENTATION, one can detect twisting of fingers.


Notes
-----

In order to stay compatible with existing applications, the data reported
in a finger packet must not be recognized as single-touch events.

For type A devices, all finger data bypasses input filtering, since
subsequent events of the same type refer to different fingers.

For example usage of the type A protocol, see the bcm5974 driver. For
example usage of the type B protocol, see the hid-egalax driver.

[1] Also, the difference (TOOL_X - POSITION_X) can be used to model tilt.
[2] The list can of course be extended.
[3] The mtdev project: http://bitmath.org/code/mtdev/.
[4] See the section on event computation.
[5] See the section on finger tracking.
