#pragma comment(linker, "/subsystem:windows")   //是编译器指令，用于指定程序的子系统为Windows，而不是控制台
#include <windows.h>
#include <iostream>
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// In_opt 与 In 的含义：这些是 Microsoft 的 SAL Annotations，用来注释函数的参数和返回值，以便代码分析和文档生成。
// In 表示输入参数，即数据传递给函数，但不被修改。In_opt 表示可选的输入参数，即可以为 NULL 或默认值
// __stdcall 关键字的含义：这是一种调用约定，用于调用 Win32 API 函数。它规定了参数的传递顺序、堆栈的清理方式、函数名的修饰方式等
// 在C语言中，常见的调用约定有__cdecl和__stdcall。
// __cdecl是C语言默认的调用约定，它的特点是：
//  函数参数从右到左依次入栈。
//  函数返回时使用ret指令。
//  由调用者手动清栈。
//  被调用的函数支持可变参数。
//  编译后的函数名为_foo，其中foo是原函数名。
// 
    /*int __cdecl sum(int a, int b);
    push 2 ; 将第二个参数压入堆栈
    push 1; 将第一个参数压入堆栈
    call _sum; 调用函数
    add esp, 8; 调用者清理堆栈*/
// __stdcall是C++的标准调用方式，它的特点是：
//  函数参数从右到左依次入栈。
//  函数返回时使用retn x指令，其中x为调整堆栈的字节数。
//  由被调用者自动清栈。
//  被调用的函数的参数个数是固定的。
//  编译后的函数名为_foo@x，其中foo是原函数名，x是参数占用的字节数。
    /*int __stdcall sum(int a, int b);
    push 2 ; 将第二个参数压入堆栈
    push 1 ; 将第一个参数压入堆栈
    call _sum@8 ; 调用函数
    ; 被调用者清理堆栈*/
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
    const char CLASS_NAME[] = "Sample Window Class";
    //  WNDCLASS和WNDCLASSEX的区别在于：
    //   WNDCLASSEX比WNDCLASS多了两个成员变量：cbSize和hIconSm。
    //   cbSize用于指定结构体的大小，用于版本检测。
    //   hIconSm用于指定窗口类的小图标的句柄，用于在任务栏和Alt + Tab切换窗口的列表中显示。
    //   WNDCLASSEX使用RegisterClassEx函数注册，而WNDCLASS使用RegisterClass函数注册。
    //   WNDCLASSEX使用GetClassInfoEx函数获取，而WNDCLASS使用GetClassInfo函数获取。
    //   一般来说，WNDCLASSEX比WNDCLASS更灵活和完善，可以提供更多的信息和功能。

    // WNDCLASS 结构体的成员变量含义：这个结构体用于存储窗口类的信息，包括以下字段：
    //  UINT style：窗口类的样式，有以下值：
    //      CS_BYTEALIGNCLIENT：将窗口的客户区域的左上角和右下角与设备上下文的最近的字节边界对齐。这样可以提高在客户区域绘制位图时的性能。
    //      CS_BYTEALIGNWINDOW：将窗口的左上角和右下角与设备上下文的最近的字节边界对齐。这样可以确保窗口的正确对齐。
    //      CS_CLASSDC：为窗口类分配一个设备上下文，所有属于该类的窗口都可以使用该设备上下文。设备上下文不会被释放，直到类被注销，或者调用GetClassInfoEx函数。
    //      CS_DBLCLKS：发送双击消息给窗口过程，当用户在窗口的客户区域内快速连续地单击鼠标左键、右键或中键时。
    //      CS_DROPSHADOW：为窗口添加一个阴影边框，使其具有三维效果。只有顶层窗口才支持该样式。
    //      CS_GLOBALCLASS：指定该窗口类是一个应用程序全局类。有关详细信息，请参阅全局类。
    //      CS_NOCLOSE：禁用窗口的关闭按钮，并在窗口菜单中移除关闭选项。
    //      CS_OWNDC：为每个窗口分配一个唯一的设备上下文。设备上下文只有在窗口被销毁时才会被释放。
    //      CS_PARENTDC：将父窗口的设备上下文设置为子窗口的剪辑区域。这样可以使子窗口的绘图操作在父窗口的客户区域内可见。
    //      CS_SAVEBITS：保存窗口下方的位图，以减少在隐藏和显示窗口时的绘图操作。只有顶层窗口才支持该样式。
    //      CS_HREDRAW：在调整窗口的大小或位置时，重绘整个窗口。V代表垂直方向（Vertical）。
    //      CS_VREDRAW：在调整窗口的大小或位置时，重绘整个窗口。H代表水平方向（Horizontal）。
    //  WNDPROC lpfnWndProc：窗口过程的指针，用于处理窗口的消息。
    //  int cbClsExtra：为该窗口类分配的额外字节数。
    //  int cbWndExtra：为该窗口类创建的每个窗口分配的额外字节数。
    //      这些额外的字节可以用于存储一些与窗口类或窗口实例相关的数据，例如一个指针或一个标志。
    //      这些额外的字节可以通过GetClassLong、SetClassLong、GetWindowLong或SetWindowLong函数来访问或修改。
    //      这些额外的字节在注册窗口类时被系统初始化为零，因此通常不需要设置它们，除非有特殊的需求。
    //  HINSTANCE hInstance：包含窗口过程的实例句柄。
    //  HICON hIcon：与该窗口类关联的图标的句柄。
    //      指定窗口类的图标句柄，用于表示窗口的图标。这个句柄必须是一个图标资源的句柄，如果为NULL，则系统会提供一个默认的图标。
    //      
    //  //  如果你想获取系统自带的问号图标的句柄，你可以使用 LoadIcon 函数，如下所示：
    //          HICON hIcon = LoadIcon(NULL, IDI_QUESTION); // 获取问号图标的句柄
    //      如果你想获取自定义的图标的句柄，你可以使用 LoadImage 函数，如下所示：(你需要将第一个参数 hInst 设置为 NULL，以指示你要从文件而不是资源中加载图像)
    //          HICON hIcon = LoadImage(NULL, "c:\\myicon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE); // 从文件中加载图标并获取句柄
    //      如果你想在运行时创建图标的句柄，你可以使用 CreateIcon 函数，如下所示：
    //          HICON hIcon = CreateIcon(hInstance, 32, 32, 1, 1, ANDmaskIcon, XORmaskIcon); // 根据位掩码创建图标并获取句柄
    //      如果你想从一个可执行文件或动态链接库中提取图标的句柄，你可以使用 ExtractIcon 函数，如下所示：
    //          HICON hIcon = ExtractIcon(hInstance, "shell32.dll", 0); // 从 shell32.dll 中提取第一个图标并获取句柄
    // 
    //  //  Windows系统自带了很多图标，它们主要存储在一些 DLL 或 EXE 文件中，你可以使用一些工具来查看或提取它们。34
    //      一些常用的包含图标的文件有：
    //          imageres.dll：包含许多 Windows 10 和 Windows 11 图标，在操作系统中几乎无处不在。具有用于不同类型文件夹、硬件设备、外围设备、操作等的图标。
    //          shell32.dll：包含许多用于 Windows 10 以及 Windows 11 各个部分的图标。在其中可以找到与互联网、设备、网络、外围设备、文件夹等相关的图标。
    //          ddores.dll：包含许多与硬件设备和资源相关的图标，例如扬声器、耳机、屏幕、计算机、遥控器、游戏手柄、鼠标和键盘、相机和打印机等。
    //          pifmgr.dll：包含一些旧版 Windows 95 和 Windows 98 中使用的旧式图标。在其中，可以找到描绘诸如窗口、小号、球和向导等事物的有趣图标帽子。
    //          explorer.exe：有一些文件资源管理器及其旧版本使用的图标。
    //  HCURSOR hCursor：与该窗口类关联的光标的句柄。
    //      指定窗口类的光标句柄，用于表示窗口的光标。这个句柄必须是一个光标资源的句柄，如果为NULL，则每当鼠标移动到窗口中时，应用程序都必须显式设置光标的形状。
    //      
    //  //  要获得光标的句柄，你可以使用 Windows API 函数 GetCursor，它可以返回当前光标的句柄。例如
    //          HCURSOR hCursor = GetCursor(); // 获取当前光标的句柄
    //      如果你想获取指定形状的光标的句柄，你可以使用 LoadCursor 或 LoadImage 函数，它们可以从文件或资源中加载光标，并返回一个 HCURSOR 类型的句柄2。例如，你可以这样写：
    //          HCURSOR hCursor = LoadCursor(NULL, IDC_ARROW); // 获取箭头形状的光标的句柄
    //      如果你想获取自定义的图标的句柄，你可以使用 LoadImage 函数，如下所示：
    //          HCURSOR hCursor = LoadImage(NULL, "c:\\mycursor.cur", IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE);  // 从文件中加载光标并获取句柄
    //      
    //  //  Windows 系统自带了很多光标，它们主要存储在一些 DLL 或 EXE 文件中，你可以使用一些工具来查看或提取它们。
    //      一些常用的包含光标的文件有：
    //      user32.dll：包含许多 Windows 10 和 Windows 11 的标准光标，如箭头、手形、等待、文本选择等。
    //      imageres.dll：包含许多 Windows 10 和 Windows 11 的动画光标，如工作中、忙碌、链接选择等。
    //      aero_cursors.dll：包含一些 Windows Vista 和 Windows 7 的光标，如 Aero 箭头、Aero 工作中、Aero 忙碌等。
    //      moricons.dll：包含一些旧版 Windows 中使用的光标，如 MS - DOS、网络、打印机等。
    //  HBRUSH hbrBackground：用于擦除窗口背景的画刷的句柄。
    //      指定窗口类的背景画笔句柄，用于填充窗口的背景。这个句柄可以是一个物理画笔的句柄，也可以是一个系统颜色值加1后转换成的画笔句柄。如果为NULL，则应用程序必须自己绘制窗口的背景。
    //  
    //  //  要获得背景的画刷的句柄，你可以使用一些 Windows API 函数，如 CreateSolidBrush，CreateHatchBrush，CreatePatternBrush 等。
    //      这些函数可以创建不同类型的画刷，并返回一个 HBRUSH 类型的句柄。例如，你可以这样写：
    //          HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0)); // 创建一个纯色的画刷
    //      你也可以使用 GetStockObject 函数来获取系统预定义的画刷的句柄，如下所示：
    //          HBRUSH hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH); // 获取一个白色的画刷
    //  LPCTSTR lpszMenuName：与该窗口类关联的菜单的名称或资源标识符。
    //      指定窗口类的菜单名称，用于与窗口类关联的菜单。这个名称可以是一个资源文件中的菜单标识符，也可以是一个字符串常量。如果为NULL，则属于该类的窗口没有默认的菜单。
    //  LPCTSTR lpszClassName：窗口类的名称。
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(255, 0, 0));
    wc.lpszClassName = CLASS_NAME;


    //  RegisterClass函数的作用是注册一个窗口类，以便在调用CreateWindow或CreateWindowEx函数时使用。
    //      窗口类是一种定义窗口的外观和行为的模板，包括窗口的样式、图标、光标、背景、菜单和窗口过程等属性。
    //  RegisterClass函数的运行过程大致如下3：
    //      首先，函数检查参数lpClassName指向的字符串或原子是否已经被其他模块注册过，
    //          如果是，则返回错误码ERROR_CLASS_ALREADY_EXISTS。
    //      然后，函数检查参数lpWndClass指向的WNDCLASS结构是否有效，
    //          如果不是，则返回错误码ERROR_INVALID_PARAMETER。
    //      接着，函数为窗口类分配一个唯一的原子，并将其与WNDCLASS结构中的属性关联起来，
    //          存储在系统的内部表中,一个原子一一对应一个字符串(窗口类名字)。
    //      最后，函数返回窗口类的原子，表示注册成功。
    //  在Windows系统中，原子是一种用于存储字符串的数据结构，它可以保证字符串的唯一性和快速访问。
    //  1.原子的工作原理是，当一个字符串被添加到原子表中时，系统会为它分配一个16位的整数值，称为原子编号，然后返回这个编号。
    //      当需要访问这个字符串时，只需提供这个编号，系统就可以快速地找到对应的字符串
    //  2.原子表字符串分配一个16位整数的过程是这样的1：
    //      首先，系统会对字符串进行哈希运算，得到一个32位的哈希值。
    //      然后，系统会将哈希值的高16位和低16位进行异或运算，得到一个16位的结果。
    //      接着，系统会检查原子表中是否已经存在该结果作为原子编号，如果是，则返回该编号；如果不是，则将该结果作为新的原子编号，并将字符串和编号存储在原子表中。
    //  3.原子的优点是，它可以节省内存空间，因为相同的字符串不会被重复存储；它也可以提高性能，因为比较原子编号比比较字符串要快得多。12
    //    原子的缺点是，它有一定的限制，例如原子表的大小、字符串的长度、原子的作用域等。
    //  4.在Windows系统中，有两种类型的原子：全局原子和本地原子。
    //      全局原子是在系统范围内有效的，可以被任何进程访问。
    //          全局原子表的大小是0x4000，即最多可以存储16384个原子。全局原子的编号范围是0xC000到0xFFFF。
    //      本地原子是在进程范围内有效的，只能被创建它们的进程访问。
    //          每个进程都有自己的本地原子表，其大小是0x2000，即最多可以存储8192个原子。本地原子的编号范围是0x0001到0xBFFF。
    RegisterClassEx(&wc);

    // AdjustWindowRectEx函数是一个Windows API函数，用于根据客户端矩形的所需大小计算窗口矩形的所需大小。
    //  lpRect：指向RECT结构的指针，该结构包含所需工作区的左上角和右下角的坐标。
    //      当函数返回时，结构包含窗口左上角和右下角的坐标，以适应所需的工作区。
    //      窗口大小与窗口内容(客户端)大小是不相同的，因为有窗口边框存在。
    //      客户端矩形是完全封闭工作区的最小矩形，而窗口矩形是完全包围窗口的最小矩形。
    //      这个函数传进去的是客户端矩形，传出来是窗口矩形。
    //  dwStyle：要计算其所需大小的窗口的窗口样式。请注意，不能指定WS_OVERLAPPED样式。
    //  bMenu：指示窗口是否具有菜单。
    //  dwExStyle：要计算其所需大小的窗口的扩展窗口样式。
    RECT windowrect;
    windowrect.left = 0;
    windowrect.top = 0;
    windowrect.right = 800;
    windowrect.bottom = 400;
    if (!AdjustWindowRectEx(&windowrect, WS_OVERLAPPEDWINDOW, false, 0))
    {
        MessageBox(NULL, "AdjustWindowRectEx failed!", "Error", MB_OK);
        return -1;
    };
    // 获得屏幕分辨率
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    // HWND是一个数据类型，它表示窗口句柄，用于唯一标识一个窗口对象。'
    // HWND是一个指向WND结构的指针，WND结构包含了窗口的属性和状态信息
    // CreateWindowEx函数的作用是创建一个具有扩展窗口样式的重叠窗口、弹出窗口或子窗口。
    //  函数的每个参数的含义如下：
    //      DWORD dwExStyle：指定窗口的扩展窗口样式，可以是Extended Window Styles中的任意组合，如WS_EX_CLIENTEDGE、WS_EX_STATICEDGE等。
    //          扩展窗口是指在多显示器配置中，将一个窗口的工作区扩展到多个显示器上的功能。
    //          扩展窗口样式可以提供一些额外的功能和效果，例如 WS_EX_APPWINDOW、WS_EX_TOPMOST 和 WS_EX_LAYERED 等1。这些样式可以使全屏窗口更加美观和实用，例如：
    //          WS_EX_APPWINDOW 可以强制窗口在任务栏上显示，即使它没有标题栏或系统菜单1。这样可以方便用户切换或关闭窗口。
    //              WS_EX_APPWINDOW 是一个扩展窗口样式，它的作用是在顶级窗口可见时强制将其放在任务栏上。
    //              顶级窗口是没有父窗口的窗口，通常是主窗口或弹出窗口。
    //              如果顶级窗口没有标题栏或系统菜单，那么它通常不会显示在任务栏上，除非使用了 WS_EX_APPWINDOW 样式
    //          WS_EX_TOPMOST 可以使窗口始终保持在最前面，即使它被停用1。这样可以避免窗口被其他窗口遮挡或干扰。
    //          WS_EX_LAYERED 可以使窗口具有半透明或透明度效果2。这样可以增加窗口的视觉效果和灵活性。
    //      LPCSTR lpClassName：指定窗口类的名称或原子，必须与RegisterClass或RegisterClassEx函数注册时使用的类名相同。
    //      LPCSTR lpWindowName：指定窗口的名称，如果窗口有标题栏，该名称将显示在标题栏中；如果窗口是控件，该名称将指定控件的文本。
    //      DWORD dwStyle：指定窗口的样式，可以是Window Styles中的任意组合，如WS_OVERLAPPED、WS_POPUP、WS_CHILD等，以及控件的特定样式，如BS_PUSHBUTTON、ES_LEFT等。
    //          WS_BORDER：创建一个带边框的窗口。
    //          WS_CAPTION：创建一个有标题栏的窗口（包括WS_BORDER风格）。
    //          WS_CHILD：创建一个子窗口。这个风格不能与WS_POPUP风格同时使用。
    //          WS_CLIPCHILDREN：当在父窗口内绘图时，排除子窗口区域。
    //          WS_CLIPSIBLINGS：排除子窗口之间的相对区域，也就是，当一个特定的窗口接收到WM_PAINT消息时，WS_CLIPSIBLINGS风格将所有层叠窗口排除在绘图之外，只重绘指定的子窗口。
    //          WS_DISABLED：创建一个初始状态为禁止的子窗口。一个禁止状态的窗口不能接受来自用户的输入信息。
    //          WS_DLGFRAME：创建一个带对话框边框风格的窗口。这种风格的窗口不能带标题栏。
    //          WS_GROUP：指定一组控件的第一个控件。这个控制组由第一个控件和随后定义的控件组成，自第二个控件开始每个控件，具有WS_GROUP风格，每个组的第一个控件带有WS_TABSTOP风格，从而使用户可以在组间移动。用户随后可以使用光标在组内的控件间改变键盘焦点。
    //          WS_HSCROLL：创建一个有水平滚动条的窗口。
    //          WS_ICONIC：创建一个初始状态为最小化状态的窗口。与WS_MINIMIZE风格相同。
    //          WS_MAXIMIZE：创建一个初始状态为最大化状态的窗口。
    //          WS_MAXIMIZEBOX：创建一个具有最大化按钮的窗口。该风格不能与WS_EX_CONTEXTHELP风格同时出现，同时必须指定WS_SYSMENU风格。
    //          WS_MINIMIZE：创建一个初始状态为最小化状态的窗口。与WS_ICONIC风格相同。
    //          WS_MINIMIZEBOX：创建一个具有最小化按钮的窗口。该风格不能与WS_EX_CONTEXTHELP风格同时出现，同时必须指定WS_SYSMENU风格。
    //          WS_OVERLAPPED：创建一个层叠的窗口。一个层叠的窗口有一个标题栏和一个边框。与WS_TILED风格相同。
    //          WS_OVERLAPPEDWINDOW：创建一个具有WS_OVERLAPPED，WS_CAPTION，WS_SYSMENU，WS_THICKFRAME，WS_MINIMIZEBOX，WS_MAXIMIZEBOX风格的层叠窗口。与WS_TILEDWINDOW风格相同。
    //          WS_POPUP：创建一个弹出窗口。这种风格不能与WS_CHILD风格同时使用。
    //              全屏窗口样式要设为 WS_POPUP 的原因是，WS_POPUP 样式的窗口是弹出窗口，它没有标题栏、边框、菜单等非工作区，因此可以占用整个屏幕的区域。
    //              如果使用其他样式的窗口，例如 WS_OVERLAPPEDWINDOW，那么窗口的尺寸会受到非工作区的影响，导致无法完全覆盖屏幕。
    //              另外，WS_POPUP 样式的窗口也可以避免与其他窗口发生重叠或遮挡，从而提高游戏的性能和体验。
    //          WS_POPUPWINDOW：创建一个具有WS_POPUP和WS_BORDER风格的弹出窗口。弹出窗口具有一个标题栏，除非指定了WS_EX_TOOLWINDOW风格。
    //          WS_SIZEBOX：创建一个具有可调边框的窗口。与WS_THICKFRAME风格相同。
    //          WS_SYSMENU：创建一个在标题栏中有一个窗口菜单的窗口。必须同时指定WS_CAPTION风格。
    //          WS_TABSTOP：创建一个控件，当用户按下Tab键时，该控件可以接收键盘焦点。按下Tab键后，键盘焦点将转移到下一个具有WS_TABSTOP风格的控件。
    //          WS_THICKFRAME：创建一个具有可调边框的窗口。与WS_SIZEBOX风格相同。
    //          WS_TILED：创建一个层叠的窗口。一个层叠的窗口有一个标题栏和一个边框。与WS_OVERLAPPED风格相同。
    //          WS_TILEDWINDOW：创建一个具有WS_OVERLAPPED，WS_CAPTION，WS_SYSMENU，WS_THICKFRAME，WS_MINIMIZEBOX，WS_MAXIMIZEBOX风格的层叠窗口。与WS_OVERLAPPEDWINDOW风格相同。
    //          WS_VISIBLE：创建一个初始状态为可见的窗口。
    //          WS_VSCROLL：创建一个有垂直滚动条的窗口。
    //      int X：指定窗口的初始水平位置，对于重叠或弹出窗口，是窗口左上角的屏幕坐标；对于子窗口，是窗口左上角相对于父窗口工作区左上角的坐标。如果设置为CW_USEDEFAULT，系统将选择窗口的默认位置。
    //      int Y：指定窗口的初始垂直位置，对于重叠或弹出窗口，是窗口左上角的屏幕坐标；对于子窗口，是窗口左上角相对于父窗口工作区左上角的坐标。如果X设置为CW_USEDEFAULT，系统将忽略Y。
    //      int nWidth：指定窗口的宽度，对于重叠窗口，是窗口的宽度，以屏幕坐标为单位；对于子窗口，是窗口的宽度，以设备单位为单位。如果设置为CW_USEDEFAULT，系统将选择窗口的默认宽度。
    //      int nHeight：指定窗口的高度，对于重叠窗口，是窗口的高度，以屏幕坐标为单位；对于子窗口，是窗口的高度，以设备单位为单位。如果nWidth设置为CW_USEDEFAULT，系统将忽略nHeight。
    //      HWND hWndParent：指定窗口的父窗口或所有者窗口的句柄，对于子窗口或拥有的窗口，必须提供有效的窗口句柄；对于弹出窗口，可以为NULL或指定父窗口；对于仅消息窗口，必须为HWND_MESSAGE或现有仅消息窗口的句柄。
    //      HMENU hMenu：指定窗口的菜单句柄或子窗口的标识符，对于重叠或弹出窗口，指定要与窗口一起使用的菜单；对于子窗口，指定子窗口的标识符，用于通知父窗口的事件。
    //      HINSTANCE hInstance：指定要与窗口关联的模块实例的句柄，通常为当前应用程序的实例句柄。
    //      LPVOID lpParam：指定要传递给窗口的值的指针，该值可以在WM_CREATE消息的lParam参数中通过CREATESTRUCT结构的lpCreateParams成员获取。如果不需要其他数据，可以为NULL。
    HWND hwnd = CreateWindowEx(
        0,                              // 没有扩展窗口样式
        CLASS_NAME,                     // 窗口类名，必须与RegisterClass或RegisterClassEx函数注册时使用的类名相同
        "Learn to Program Windows",     // 窗口标题，将显示在标题栏中
        WS_OVERLAPPEDWINDOW,            // 窗口样式，包括边框、标题栏、菜单栏、最小化和最大化按钮等
        CW_USEDEFAULT, CW_USEDEFAULT,   // 窗口的初始位置，由系统选择默认位置
        windowrect.right - windowrect.left, 
        windowrect.bottom-windowrect.top,
        NULL,                           // 没有父窗口或所有者窗口
        NULL,                           // 没有菜单
        hInstance,                      // 模块实例句柄，通常为当前应用程序的实例句柄
        NULL                            // 没有额外的数据
    );
    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    // 而创建前台窗口的线程叫做前台线程（或叫前景线程），前台线程拥有比其它非前景线程更高的优先级，会被windows操作系统优先处理。
    // 而所谓的焦点窗口，可以是前台窗口或者是前台窗口的子窗口（控件），如果用户按下键盘按键，windows操作系统会将键盘按键消息发送到当前的焦点窗口。
    
    // SetForegroundWindow 函数是用来将一个窗口设置为前台窗口，也就是用户当前操作的窗口。
    //	前台窗口是 Z-序的顶部窗口，它会接收键盘输入，并且有一些视觉提示，例如任务栏按钮的闪烁。
    //	系统也会为创建前台窗口的线程分配略高的优先级。
    // SetFocus 函数是用来将键盘焦点设置到一个窗口，也就是用户当前输入的窗口。
    //	焦点窗口可以是前台窗口，也可以是前台窗口的子窗口（控件）。
    //	系统会将键盘消息发送到焦点窗口，以便它可以处理用户的输入。
    
    // SetForegroundWindow 和 SetFocus 之间的关系是，当一个窗口被设置为前台窗口时，它也会自动获得键盘焦点，除非它有一个子窗口已经拥有焦点。
    //	如果一个窗口只是被设置为焦点窗口，而不是前台窗口，那么它不会改变 Z-序，也不会有视觉提示，而且它的线程优先级也不会提高。
    // SetForegroundWindow 和 SetFocus 的共同功能是，它们都可以使一个窗口接收用户的键盘输入。
    //	但是，它们的区别是，SetForegroundWindow 还会改变窗口的 Z - 序和线程的优先级，而 SetFocus 只会改变键盘焦点
    
    // 函数的作用是将创建指定窗口的线程设置到前台，并且激活该窗口。
    // 系统给创建前台窗口的线程分配的优先级稍高于其他线程的优先级。SetForegroundWindow 会影响窗口的 Z 顺序，即窗口在屏幕上的堆叠顺序。
    SetForegroundWindow(hwnd);
    //  函数的作用是将键盘焦点(可接受键盘消息)设置到指定的窗口。该窗口必须附属于调用线程的消息队列，否则函数会失败。
    //		键盘焦点是指当前可以接收键盘输入的窗口或控件。
    //		在 Windows 中，一般有键盘焦点的窗口或控件会有一些可视的提示，例如窗口的标题栏会被加亮，控件会有一个字符插入符或一个虚线框。
    //		整个桌面在某个时刻只可能有一个键盘焦点，用户可以通过鼠标点击或 Tab 键来切换键盘焦点。
    //	此函数向失去键盘焦点的窗口发送 WM_KILLFOCUS 消息，向接收键盘焦点的窗口发送 WM_SETFOCUS 消息。它还会激活接收焦点的窗口或接收焦点的窗口的父级。
    SetFocus(hwnd);
    UpdateWindow(hwnd);
    // GetMessage函数是用于从调用线程的消息队列中检索消息的Windows API函数。它会阻塞线程，直到有消息可用为止。
    //  如果函数检索到WM_QUIT消息，它会返回0，表示应用程序应该结束主消息循环并退出。
    //  如果函数检索到其他消息，它会返回非零值，并将消息信息存储在MSG结构中。
    //  如果出现错误，它会返回 - 1。
    // TranslateMessage函数是用于将虚拟键消息转换为字符消息的Windows API函数。
    //  它会根据键盘驱动程序的映射，将WM_KEYDOWN和WM_KEYUP消息的组合转换为WM_CHAR或WM_DEADCHAR消息，
    //  并将新的消息投递到调用线程的消息队列中。它不会修改原有的消息，也不会对非键盘消息进行转换。
    //  它通常用于将键盘输入转换为可打印的字符。
    // DispatchMessage函数是用于将消息分发到窗口过程的Windows API函数。
    //  它会根据MSG结构中的窗口句柄和消息值，调用相应的窗口过程函数来处理消息。
    //  它通常用于分发GetMessage或PeekMessage函数检索到的消息。
    //  它会返回窗口过程的返回值，但通常这个值会被忽略5。

    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
    AllocConsole();
    SetConsoleTitle("调试窗口");
    FILE* stream;
    freopen_s(&stream, "CONIN$", "r", stdin);
    freopen_s(&stream, "CONOUT$", "w+", stdout);
    freopen_s(&stream, "CONOUT$", "w+", stderr);


    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

// WindowProc函数是一个窗口过程函数，用于处理发送到窗口的消息。它有四个参数：
//  HWND hwnd：窗口的句柄，用于标识窗口的实例。
//  UINT uMsg：消息的代码，用于指示消息的类型，如WM_SIZE，WM_PAINT等。
//      UNIT uMsg是一个无符号整数类型，表示发送到窗口的消息的代码。
//      它用于指示消息的类型，如窗口的创建，销毁，移动，大小，绘制，键盘，鼠标，菜单等。
//      它有很多种可能的值，每种值都有不同的含义和用途。以下是一些常见的uMsg值的含义：
//      WM_CREATE：当窗口被创建时发送的消息，用于执行一些初始化的操作。
//      WM_DESTROY：当窗口被销毁时发送的消息，用于执行一些清理的操作，并向消息队列中发送一个WM_QUIT消息，以结束消息循环。
//      WM_MOVE：当窗口被移动时发送的消息，用于获取窗口的新位置。
//      WM_SIZE：当窗口被调整大小时发送的消息，用于获取窗口的新大小，并重新布局子控件。
//      WM_PAINT：当窗口需要重绘时发送的消息，用于执行一些绘图的操作，并清除窗口的更新区域。
//      WM_CLOSE：当用户试图关闭窗口时发送的消息，用于询问用户是否确定关闭，并销毁窗口。
//      WM_QUIT：当应用程序应该结束时发送的消息，用于退出消息循环，并返回一个退出码。
//      WM_KEYDOWN：当用户按下一个非系统键时发送的消息，用于获取按键的虚拟键码和重复次数。
//      WM_KEYUP：当用户释放一个非系统键时发送的消息，用于获取按键的虚拟键码和重复次数。
//      WM_CHAR：当用户按下一个可打印的字符键时发送的消息，用于获取字符的ASCII码和重复次数。
//      WM_MOUSEMOVE：当鼠标在窗口内移动时发送的消息，用于获取鼠标的位置和按键状态。
//      WM_LBUTTONDOWN：当用户按下鼠标左键时发送的消息，用于获取鼠标的位置和按键状态。
//      WM_LBUTTONUP：当用户释放鼠标左键时发送的消息，用于获取鼠标的位置和按键状态。
//      WM_LBUTTONDBLCLK：当用户双击鼠标左键时发送的消息，用于获取鼠标的位置和按键状态。
//      WM_RBUTTONDOWN：当用户按下鼠标右键时发送的消息，用于获取鼠标的位置和按键状态。
//      WM_RBUTTONUP：当用户释放鼠标右键时发送的消息，用于获取鼠标的位置和按键状态。
//      WM_RBUTTONDBLCLK：当用户双击鼠标右键时发送的消息，用于获取鼠标的位置和按键状态。
//      WM_MBUTTONDOWN：当用户按下鼠标中键时发送的消息，用于获取鼠标的位置和按键状态。
//      WM_MBUTTONUP：当用户释放鼠标中键时发送的消息，用于获取鼠标的位置和按键状态。
//      WM_MBUTTONDBLCLK：当用户双击鼠标中键时发送的消息，用于获取鼠标的位置和按键状态。
//      WM_MOUSEWHEEL：当用户滚动鼠标滚轮时发送的消息，用于获取鼠标的位置和滚动值。
//      WM_COMMAND：当用户选择一个菜单项或操作一个子控件时发送的消息，用于获取菜单或控件的标识符和通知码。
//  WPARAM wParam：消息的第一个参数，用于传递与消息相关的数据，其含义取决于消息的代码。
//  LPARAM lParam：消息的第二个参数，用于传递与消息相关的数据，其含义取决于消息的代码。

// WindowProc函数在以下情况下被调用：
//  当窗口收到一个消息时，DispatchMessage函数会根据窗口句柄和消息代码，调用相应的窗口过程函数来处理消息。
//  当窗口需要执行一些默认的操作时，可以在窗口过程中显式地调用DefWindowProc函数，它会调用系统提供的默认窗口过程函数。
//  当窗口需要调用另一个窗口的窗口过程时，可以在窗口过程中显式地调用CallWindowProc函数，它会调用指定的窗口过程函数。
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_MOUSEMOVE: 
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        std::cout << "x=" << x << " " << "y=" << y << std::endl;

        return 0;
    }
    case WM_DESTROY:
        // PostQuitMessage 函数的作用是向系统指示线程已发出终止请求，即退出。
        //  它通常用于响应 WM_DESTROY 消息。
        //  PostQuitMessage 函数将 WM_QUIT 消息发布到线程的消息队列并立即返回;
        //  函数只是向系统指示线程正在请求在将来的某个时间退出。
        //  当线程从其消息队列中检索 WM_QUIT 消息时，它应退出其消息循环，并将控制权返回到系统。
        //  返回到系统的退出值必须是 WM_QUIT 消息的 wParam 参数
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
      
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)GetClassLongPtr(hwnd, GCLP_HBRBACKGROUND));

        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
