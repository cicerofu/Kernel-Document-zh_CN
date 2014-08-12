# 多点触摸协议

> https://www.kernel.org/doc/Documentation/input/multi-touch-protocol.txt

## 介绍

要想使用强大的多点触摸设备，就需要了解如何读取多个触摸点的详细数据，例如，直接
触摸设备表面获取的对象。这篇文档描述多点触摸（以下简称MT）协议，让内核驱动读取
多触摸点的详细信息。

根据设备功能，协议分为两类。匿名触摸（A类），向接收器发送所有触摸点的raw数据。
可跟踪id触摸（B类），通过事件槽向每个触摸点发送独自的更新数据。


## 协议的使用

触摸点的详细数据被当作独立的ABS_MT事件数据包顺序发送。只有ABS_MT事件才能被识别
为一个触摸数据包的一部分。虽然多点触摸事件被单点（ST）应用程序所忽略，但是多点
触摸协议依然可以在现有的单点触摸协议驱动之上得以实现。

A类驱动在数据包结尾处通过调用input_mt_sync()来区别单个触摸数据包。产生一个
SYN_MT_REPORT事件，通知接收器接受当前触摸的数据，并准备接收下一个数据。

B类驱动在数据包开头处通过调用input_mt_slot()，槽作为参数，来区别单个触摸数据
包。产生一个ABS_MT_SLOT事件，通知接收器准备更新当前槽。

A、B类驱动都是通过调用input_sync()来标识多点触摸会话的结束。通知接收器执行
EV_SYN/SYN_REPORT之前的所有事件，并准备接收新一轮的信号/数据包。

无状态的A类协议和有状态的B类槽协议的主要区别是使用触摸id，减少了向用户态发送的
数据量。槽协议需要使用ABS_MT_TRACKING_ID，由硬件提供，或从raw数据中获取[5]。

在A类设备表面触摸，内核驱动会产生无序的当前匿名触摸数据。出现在事件流里的数据包
没有先后顺序。事件过滤、手指跟踪留给用户态来实现[3]。

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
最大值更多的触摸总数，多于B类槽报告的ABS_MT_SLOT矩阵的absinfo触摸总数。

ABS_MT_SLOT矩阵的最小值必须是0。

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


Protocol Example B
------------------

Here is what a minimal event sequence for a two-contact touch would look
like for a type B device:

   ABS_MT_SLOT 0
   ABS_MT_TRACKING_ID 45
   ABS_MT_POSITION_X x[0]
   ABS_MT_POSITION_Y y[0]
   ABS_MT_SLOT 1
   ABS_MT_TRACKING_ID 46
   ABS_MT_POSITION_X x[1]
   ABS_MT_POSITION_Y y[1]
   SYN_REPORT

Here is the sequence after moving contact 45 in the x direction:

   ABS_MT_SLOT 0
   ABS_MT_POSITION_X x[0]
   SYN_REPORT

Here is the sequence after lifting the contact in slot 0:

   ABS_MT_TRACKING_ID -1
   SYN_REPORT

The slot being modified is already 0, so the ABS_MT_SLOT is omitted.  The
message removes the association of slot 0 with contact 45, thereby
destroying contact 45 and freeing slot 0 to be reused for another contact.

Finally, here is the sequence after lifting the second contact:

   ABS_MT_SLOT 1
   ABS_MT_TRACKING_ID -1
   SYN_REPORT


Event Usage
-----------

A set of ABS_MT events with the desired properties is defined. The events
are divided into categories, to allow for partial implementation.  The
minimum set consists of ABS_MT_POSITION_X and ABS_MT_POSITION_Y, which
allows for multiple contacts to be tracked.  If the device supports it, the
ABS_MT_TOUCH_MAJOR and ABS_MT_WIDTH_MAJOR may be used to provide the size
of the contact area and approaching tool, respectively.

The TOUCH and WIDTH parameters have a geometrical interpretation; imagine
looking through a window at someone gently holding a finger against the
glass.  You will see two regions, one inner region consisting of the part
of the finger actually touching the glass, and one outer region formed by
the perimeter of the finger. The center of the touching region (a) is
ABS_MT_POSITION_X/Y and the center of the approaching finger (b) is
ABS_MT_TOOL_X/Y. The touch diameter is ABS_MT_TOUCH_MAJOR and the finger
diameter is ABS_MT_WIDTH_MAJOR. Now imagine the person pressing the finger
harder against the glass. The touch region will increase, and in general,
the ratio ABS_MT_TOUCH_MAJOR / ABS_MT_WIDTH_MAJOR, which is always smaller
than unity, is related to the contact pressure. For pressure-based devices,
ABS_MT_PRESSURE may be used to provide the pressure on the contact area
instead. Devices capable of contact hovering can use ABS_MT_DISTANCE to
indicate the distance between the contact and the surface.


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


In addition to the MAJOR parameters, the oval shape of the touch and finger
regions can be described by adding the MINOR parameters, such that MAJOR
and MINOR are the major and minor axis of an ellipse. The orientation of
the touch ellipse can be described with the ORIENTATION parameter, and the
direction of the finger ellipse is given by the vector (a - b).

For type A devices, further specification of the touch shape is possible
via ABS_MT_BLOB_ID.

The ABS_MT_TOOL_TYPE may be used to specify whether the touching tool is a
finger or a pen or something else. Finally, the ABS_MT_TRACKING_ID event
may be used to track identified contacts over time [5].

In the type B protocol, ABS_MT_TOOL_TYPE and ABS_MT_TRACKING_ID are
implicitly handled by input core; drivers should instead call
input_mt_report_slot_state().


Event Semantics
---------------

ABS_MT_TOUCH_MAJOR

The length of the major axis of the contact. The length should be given in
surface units. If the surface has an X times Y resolution, the largest
possible value of ABS_MT_TOUCH_MAJOR is sqrt(X^2 + Y^2), the diagonal [4].

ABS_MT_TOUCH_MINOR

The length, in surface units, of the minor axis of the contact. If the
contact is circular, this event can be omitted [4].

ABS_MT_WIDTH_MAJOR

The length, in surface units, of the major axis of the approaching
tool. This should be understood as the size of the tool itself. The
orientation of the contact and the approaching tool are assumed to be the
same [4].

ABS_MT_WIDTH_MINOR

The length, in surface units, of the minor axis of the approaching
tool. Omit if circular [4].

The above four values can be used to derive additional information about
the contact. The ratio ABS_MT_TOUCH_MAJOR / ABS_MT_WIDTH_MAJOR approximates
the notion of pressure. The fingers of the hand and the palm all have
different characteristic widths.

ABS_MT_PRESSURE

The pressure, in arbitrary units, on the contact area. May be used instead
of TOUCH and WIDTH for pressure-based devices or any device with a spatial
signal intensity distribution.

ABS_MT_DISTANCE

The distance, in surface units, between the contact and the surface. Zero
distance means the contact is touching the surface. A positive number means
the contact is hovering above the surface.

ABS_MT_ORIENTATION

The orientation of the touching ellipse. The value should describe a signed
quarter of a revolution clockwise around the touch center. The signed value
range is arbitrary, but zero should be returned for an ellipse aligned with
the Y axis of the surface, a negative value when the ellipse is turned to
the left, and a positive value when the ellipse is turned to the
right. When completely aligned with the X axis, the range max should be
returned.

Touch ellipsis are symmetrical by default. For devices capable of true 360
degree orientation, the reported orientation must exceed the range max to
indicate more than a quarter of a revolution. For an upside-down finger,
range max * 2 should be returned.

Orientation can be omitted if the touch area is circular, or if the
information is not available in the kernel driver. Partial orientation
support is possible if the device can distinguish between the two axis, but
not (uniquely) any values in between. In such cases, the range of
ABS_MT_ORIENTATION should be [0, 1] [4].

ABS_MT_POSITION_X

The surface X coordinate of the center of the touching ellipse.

ABS_MT_POSITION_Y

The surface Y coordinate of the center of the touching ellipse.

ABS_MT_TOOL_X

The surface X coordinate of the center of the approaching tool. Omit if
the device cannot distinguish between the intended touch point and the
tool itself.

ABS_MT_TOOL_Y

The surface Y coordinate of the center of the approaching tool. Omit if the
device cannot distinguish between the intended touch point and the tool
itself.

The four position values can be used to separate the position of the touch
from the position of the tool. If both positions are present, the major
tool axis points towards the touch point [1]. Otherwise, the tool axes are
aligned with the touch axes.

ABS_MT_TOOL_TYPE

The type of approaching tool. A lot of kernel drivers cannot distinguish
between different tool types, such as a finger or a pen. In such cases, the
event should be omitted. The protocol currently supports MT_TOOL_FINGER and
MT_TOOL_PEN [2]. For type B devices, this event is handled by input core;
drivers should instead use input_mt_report_slot_state().

ABS_MT_BLOB_ID

The BLOB_ID groups several packets together into one arbitrarily shaped
contact. The sequence of points forms a polygon which defines the shape of
the contact. This is a low-level anonymous grouping for type A devices, and
should not be confused with the high-level trackingID [5]. Most type A
devices do not have blob capability, so drivers can safely omit this event.

ABS_MT_TRACKING_ID

The TRACKING_ID identifies an initiated contact throughout its life cycle
[5]. The value range of the TRACKING_ID should be large enough to ensure
unique identification of a contact maintained over an extended period of
time. For type B devices, this event is handled by input core; drivers
should instead use input_mt_report_slot_state().


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
