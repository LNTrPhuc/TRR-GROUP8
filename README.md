DIJKSTRA SIMULATOR — HƯỚNG DẪN CHẠY PROJECT

Project mô phỏng thuật toán Dijkstra bằng C++20 và Qt 6 Widgets, build bằng CMake. Hướng dẫn dành cho Windows 64-bit, sử dụng CLion hoặc VS Code.

Cài môi trường ở mục 2 trước, sau đó chọn làm mục 3 nếu dùng CLion hoặc mục 4 nếu dùng VS Code.

1. GIỚI THIỆU PROJECT

Chức năng

Mô phỏng từng bước, quay lại bước trước và chạy tự động.
Chọn nguồn, đích hoặc tìm khoảng cách từ nguồn đến mọi đỉnh.
Hiển thị khoảng cách, đỉnh trước, hàng đợi ưu tiên và code thuật toán.
Nhập đồ thị TXT, dùng đồ thị mẫu hoặc tạo đồ thị ngẫu nhiên.
Chỉnh sửa đồ thị vô hướng, có hướng và hỗn hợp ngay trên giao diện.

Các file và thư mục cần biết

CMakeLists.txt ở thư mục gốc: file mở đầu quá trình build.
source: chứa mã nguồn và tài nguyên giao diện.
source/main.cpp: khởi tạo ứng dụng.
source/MainWindow.cpp: giao diện và các nút điều khiển.
source/DijkstraSimulator.cpp: thuật toán và đọc dữ liệu đồ thị.
source/GraphWidget.cpp: vẽ và tương tác với đồ thị.
source/PathPreview.cpp: xem trước đường đi.
examples: chứa các file đồ thị mẫu để chạy thử.

Luôn mở thư mục gốc chứa CMakeLists.txt, không chỉ mở riêng main.cpp. Target cần chạy có tên DijkstraSimulator.


2. CÀI MÔI TRƯỜNG BẰNG MSYS2

Tải và cài MSYS2

Mở trình duyệt, truy cập https://www.msys2.org/.
Trong phần Installation, tải bộ cài Windows x86_64.
Mở file vừa tải, bấm Next.
Giữ thư mục cài mặc định là C:\msys64 để làm theo các đường dẫn trong tài liệu.
Bấm Next và hoàn tất cài đặt.
Mở menu Start của Windows.
Tìm MSYS2 UCRT64 và mở ứng dụng này.
Kiểm tra cửa sổ terminal đang dùng môi trường UCRT64. Các lệnh cài đặt ở phần sau chạy trong cửa sổ này.

Tên phần mềm chính xác là MSYS2. Hướng dẫn dùng bộ UCRT64 để Qt, GCC, GDB, CMake và Ninja cùng một môi trường.

Cập nhật MSYS2

Trong cửa sổ MSYS2 UCRT64, nhập lệnh sau rồi nhấn Enter:

    pacman -Syu

Nếu được hỏi có tiếp tục không, nhập Y rồi nhấn Enter.
Nếu chương trình yêu cầu đóng terminal sau cập nhật thành phần lõi, làm theo thông báo rồi mở lại MSYS2 UCRT64 từ menu Start.
Chạy lại pacman -Syu và chờ cập nhật hoàn tất. Nếu có yêu cầu khởi động lại terminal, lặp lại thao tác đến khi không còn bản cập nhật.

Cài compiler, Qt và các công cụ

Trong MSYS2 UCRT64, sao chép toàn bộ dòng sau rồi nhấn Enter:

    pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-gdb mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-qt6-base

Nhập Y rồi nhấn Enter nếu được hỏi xác nhận cài đặt.
Chờ tải và cài xong, không đóng cửa sổ khi đang cài.
Mở File Explorer, vào C:\msys64\ucrt64\bin.
Kiểm tra có g++.exe, gcc.exe, gdb.exe, cmake.exe và ninja.exe.
Kiểm tra thư mục C:\msys64\ucrt64\lib\cmake\Qt6 có Qt6Config.cmake.

Không lấy compiler từ một bộ MinGW khác để ghép với Qt UCRT64 vừa cài. Các đường dẫn bên dưới đều dùng C:\msys64\ucrt64. Nếu bạn chọn nơi cài MSYS2 khác, thay đường dẫn tương ứng.

Thêm công cụ vào PATH của Windows

Bấm Start của Windows.
Nhập Edit environment variables for your account rồi mở kết quả cùng tên.
Trong phần User variables, chọn biến Path.
Bấm Edit.
Bấm New.
Nhập C:\msys64\ucrt64\bin.
Đưa dòng vừa thêm lên trước các đường dẫn Qt hoặc MinGW khác nếu máy đang có nhiều bộ công cụ.
Bấm OK để đóng cửa sổ chỉnh Path.
Trong phần User variables, bấm New để thêm biến giúp CMake tìm Qt.
Tại Variable name, nhập CMAKE_PREFIX_PATH.
Tại Variable value, nhập C:/msys64/ucrt64.
Bấm OK. Nếu biến này đã tồn tại và đang phục vụ project khác, ghi lại giá trị cũ; có thể thêm đường dẫn UCRT64 vào đầu, cách các đường dẫn bằng dấu chấm phẩy.
Bấm OK để lưu và đóng cửa sổ Environment Variables.
Đóng hoàn toàn CLion, VS Code và các terminal đang mở, sau đó mở lại để nhận môi trường mới.

Không thêm C:\msys64\usr\bin thay cho ucrt64\bin. Đây là hai môi trường khác nhau.

Kiểm tra trước khi mở project

Mở menu Start, tìm PowerShell rồi mở.
Nhập từng lệnh dưới đây, nhấn Enter sau mỗi lệnh:

    g++ --version
    gdb --version
    cmake --version
    ninja --version

Mỗi lệnh cần hiển thị phiên bản công cụ, không báo không tìm thấy.
Nhập lệnh sau để kiểm tra compiler đang được ưu tiên:

    where.exe g++

Dòng đầu tiên cần trỏ tới C:\msys64\ucrt64\bin\g++.exe. Nếu khác, kiểm tra thứ tự Path ở phần “Thêm công cụ vào PATH của Windows” rồi mở lại PowerShell.

Chuẩn bị project

Giải nén project vào thư mục có quyền ghi.
Kiểm tra có CMakeLists.txt, source và examples.
Không dùng lại thư mục cmake-build-debug đi kèm từ máy khác vì có thể chứa đường dẫn cũ.
CLion sẽ dùng build-clion-ucrt64; VS Code sẽ dùng build-vscode-ucrt64 theo hướng dẫn bên dưới.


3. CHẠY PROJECT TRÊN CLION

Mở project

Mở CLion.
Bấm Open.
Chọn thư mục gốc project chứa CMakeLists.txt.
Bấm nút mở thư mục.
Nếu CMake báo lỗi ngay lần đầu, tiếp tục cấu hình các bước sau.

Chọn bộ công cụ MSYS2

Trên thanh menu, bấm File.
Bấm Settings.
Ở danh sách bên trái, mở rộng Build, Execution, Deployment.
Bấm Toolchains.
Bấm dấu cộng rồi chọn MinGW.
Tại Name, nhập MSYS2-UCRT64.
Tại Toolset hoặc MinGW, chọn thư mục C:\msys64\ucrt64.
Tại C Compiler, chọn C:\msys64\ucrt64\bin\gcc.exe.
Tại C++ Compiler, chọn C:\msys64\ucrt64\bin\g++.exe.
Tại Debugger, chọn C:\msys64\ucrt64\bin\gdb.exe.
Tại CMake, chọn C:\msys64\ucrt64\bin\cmake.exe.
Tại Build Tool, chọn C:\msys64\ucrt64\bin\ninja.exe.
Chờ CLion nhận diện công cụ, kiểm tra không có đường dẫn báo lỗi.
Bấm Apply và giữ cửa sổ Settings mở.

Tên một số trường có thể khác đôi chút tùy phiên bản CLion.

Mở CMake options và khai báo Qt

Trong danh sách bên trái của Settings, tìm nhóm Build, Execution, Deployment.
Bấm CMake trong nhóm này.
Chọn profile Debug. Nếu chưa có, bấm dấu cộng để thêm profile và đặt Name là Debug.
Tại Build type, chọn Debug.
Tại Toolchain, chọn MSYS2-UCRT64 vừa tạo.
Tại Generator, chọn Ninja.
Tại Build directory, nhập build-clion-ucrt64.
Tìm ô CMake options của profile đang chọn.
Bấm vào ô này và nhập:

    -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64

Nếu ô đã có các tùy chọn cần giữ, thêm nội dung trên sau một khoảng trắng.
Bấm Apply.
Bấm OK để đóng Settings.
Trên thanh menu, bấm Tools, bấm CMake, chọn Reload CMake Project. Nếu CLion tự tải lại CMake, chờ quá trình đó hoàn tất.
Kiểm tra CMake không còn báo lỗi tìm Qt và đã nhận diện target DijkstraSimulator.

Build và chạy

Mở danh sách cấu hình chạy ở thanh công cụ phía trên.
Chọn DijkstraSimulator. Nếu đã có cấu hình này, chuyển đến thao tác bấm menu Build bên dưới.
Nếu chưa có, bấm menu Run.
Bấm Edit Configurations.
Bấm dấu cộng và chọn CMake Application.
Tại Name, nhập DijkstraSimulator.
Tại Target, chọn DijkstraSimulator.
Nếu có ô Executable, chọn chương trình DijkstraSimulator tương ứng.
Tại Working directory, chọn thư mục gốc project chứa CMakeLists.txt. Chọn profile Debug nếu cửa sổ có mục chọn profile.
Bấm Apply rồi bấm OK.
Bấm menu Build, chọn Build Project.
Chờ build hoàn tất, không còn thông báo lỗi.
Bấm nút Run hình tam giác xanh của cấu hình DijkstraSimulator.
Cửa sổ “Dijkstra — Mô phỏng đường đi ngắn nhất” xuất hiện là chạy thành công.

File thực thi nằm tại build-clion-ucrt64/source/DijkstraSimulator.exe khi dùng cấu hình Ninja trên.

Debug khi cần

Mở source/main.cpp.
Bấm vào lề cạnh số dòng tại dòng có thể thực thi để đặt breakpoint.
Chọn cấu hình DijkstraSimulator.
Bấm Debug.
Khi chương trình dừng, dùng Step Over để chạy dòng tiếp theo hoặc Continue để tiếp tục.
Xem Variables để theo dõi giá trị biến, bấm Stop để kết thúc.


4. CHẠY PROJECT TRÊN VS CODE

Phần này dùng MSYS2 đã cài ở mục 2. Thực hiện qua các nút và ô cài đặt của VS Code, không cần tự viết file cấu hình.

Cài extension

Mở VS Code.
Nhấn Ctrl+Shift+X để mở Extensions.
Trong ô tìm kiếm, nhập ms-vscode.cpptools.
Chọn C/C++ do Microsoft phát hành, bấm Install.
Tìm tiếp ms-vscode.cmake-tools.
Chọn CMake Tools do Microsoft phát hành, bấm Install.
Nếu được yêu cầu, bấm Reload Window.

Mở project

Bấm menu File.
Bấm Open Folder.
Chọn thư mục gốc chứa CMakeLists.txt.
Bấm Select Folder.
Kiểm tra Explorer hiển thị source, examples và CMakeLists.txt.

Chọn CMake và thư mục build bằng giao diện Settings

Bấm menu File.
Bấm Preferences, sau đó bấm Settings. Có thể nhấn Ctrl và dấu phẩy để mở nhanh.
Chọn thẻ Workspace để thiết lập áp dụng cho project này.
Trong ô tìm kiếm, nhập CMake Path.
Tìm mục Cmake: Cmake Path và nhập C:/msys64/ucrt64/bin/cmake.exe.
Xóa nội dung ô tìm kiếm, nhập CMake Generator.
Tại Cmake: Generator, nhập Ninja.
Xóa ô tìm kiếm, nhập CMake Build Directory.
Tại Cmake: Build Directory, nhập ${workspaceFolder}/build-vscode-ucrt64.
Xóa ô tìm kiếm, nhập CMake Use CMake Presets.
Tại Cmake: Use CMake Presets, chọn never để dùng cách chọn compiler bằng kit ở bước tiếp theo.
Đóng thẻ Settings. VS Code tự lưu các thay đổi.

Qt được tìm qua biến CMAKE_PREFIX_PATH đã thêm ở phần “Thêm công cụ vào PATH của Windows”. Nếu vừa sửa biến môi trường, đóng hoàn toàn VS Code rồi mở lại trước khi configure.

Chọn compiler và build

Nhấn Ctrl+Shift+P để mở ô lệnh.
Nhập CMake: Scan for Kits, chọn lệnh cùng tên rồi nhấn Enter.
Chờ quét xong, nhấn Ctrl+Shift+P lần nữa.
Nhập CMake: Select a Kit, chọn lệnh rồi nhấn Enter.
Chọn GCC tương ứng C:\msys64\ucrt64\bin\g++.exe. Tên kit có thể kèm phiên bản GCC.
Không chọn compiler từ CLion bundled MinGW, MSVC hoặc một thư mục MinGW khác cho bộ Qt UCRT64 này.
Nhấn Ctrl+Shift+P, chạy CMake: Select Variant.
Chọn Debug.
Nhấn Ctrl+Shift+P, chạy CMake: Configure.
Chờ cấu hình hoàn tất. Nếu có lỗi, đọc mục 7 trước khi tiếp tục.
Nhấn Ctrl+Shift+P, chạy CMake: Build.
Nếu cần chọn build target, chọn DijkstraSimulator hoặc all.
Chờ build xong, kiểm tra không còn lỗi.

Chạy ứng dụng

Nhấn Ctrl+Shift+P.
Chạy CMake: Set Debug Target.
Chọn DijkstraSimulator.
Nhấn Ctrl+Shift+P lần nữa.
Chạy CMake: Run Without Debugging.
Chờ cửa sổ ứng dụng xuất hiện.

File thực thi nằm tại build-vscode-ucrt64/source/DijkstraSimulator.exe.

Không dùng Run Code của Code Runner để chạy riêng main.cpp vì project cần build toàn bộ bằng CMake và liên kết Qt.

Debug khi cần

Mở source/main.cpp trong Explorer.
Bấm vào lề cạnh số dòng tại dòng có thể thực thi để đặt breakpoint.
Nhấn Ctrl+Shift+P.
Chạy CMake: Debug.
Chọn DijkstraSimulator nếu được hỏi target.
Khi chương trình dừng, dùng Continue, Step Over hoặc Step Into trên thanh debug.
Xem Variables để quan sát biến và bấm Stop để kết thúc.

Nếu không tìm thấy GDB, kiểm tra C:\msys64\ucrt64\bin\gdb.exe và PATH theo mục 2, sau đó mở lại VS Code.


5. SỬ DỤNG VÀ KIỂM TRA KẾT QUẢ

Chạy thử đồ thị mẫu

Mở ứng dụng, bấm Insert.
Chọn examples/do_thi_mau.txt.
Chọn Nguồn là a.
Chọn Đích là d.
Bấm Bước tiếp để xem từng bước hoặc Chạy tự động để chạy đến kết quả.
Kết quả mong đợi là a → c → d, tổng trọng số 30.
Đổi Đích thành Mọi đỉnh để xem khoảng cách từ nguồn đến tất cả đỉnh.

Với examples/do_thi_hon_hop.txt, chọn nguồn A và đích F. Kết quả mong đợi là A → C → B → D → E → F, tổng trọng số 13.

Các nút điều khiển

Danh sách đồ thị mẫu: chọn một mẫu hoặc bảng trắng.
Tạo ngẫu nhiên: tạo đồ thị với các tham số tùy chọn.
Insert: mở file TXT chứa đồ thị.
Nguồn và Đích: chọn bài toán cần mô phỏng.
Chạy tự động hoặc phím Space: chạy hoặc tạm dừng.
Bước tiếp và Quay lại: tiến hoặc lùi một bước mô phỏng.
Đặt lại: đưa mô phỏng về trạng thái ban đầu.
Thanh Tốc độ: điều chỉnh tốc độ chạy.
Mở code thuật toán: xem phần code và dòng đang xử lý.

Thao tác với đồ thị

Kéo trên vùng trắng để chọn nhiều đối tượng.
Giữ Ctrl khi chọn để thêm hoặc bớt đối tượng khỏi vùng chọn.
Bấm Backspace để xóa đối tượng đã chọn.
Bấm đúp vào đỉnh hoặc trọng số để chỉnh sửa.
Chọn đỉnh rồi kéo bằng chuột trái để nối cạnh.
Giữ Alt và kéo để di chuyển.
Lăn chuột để phóng to hoặc thu nhỏ.
Giữ nút chuột giữa và kéo để dịch khung nhìn.

Ký hiệu ∞ có thể là chưa tìm thấy đường ở bước hiện tại hoặc không thể tới đỉnh đó khi thuật toán kết thúc. Cạnh có hướng chỉ đi được theo chiều mũi tên.


6. CÁCH VIẾT FILE ĐỒ THỊ

Cú pháp từng cạnh

A B 10 nghĩa là cạnh vô hướng A — B, trọng số 10.
A -- B 10 cũng là cạnh vô hướng.
A -> B 10 nghĩa là cạnh có hướng từ A đến B.
A B 10 -> là cách viết khác của cạnh có hướng.
A B 10 -- là cách viết khác của cạnh vô hướng.

Ví dụ nội dung file

    Circle
    A -> B 6
    A C 2
    B C 1
    B -> D 5
    D -> E 2
    E -> F 3

Quy tắc nhập

Mỗi cạnh nằm trên một dòng, các trường cách nhau bằng khoảng trắng.
Dòng nội dung đầu tiên có thể là Circle, Square, Grid hoặc Freedom để chọn bố cục. Không ghi thì dùng bố cục lưới.
Không cần ghi số đỉnh và số cạnh ở đầu file.
Tên đỉnh không chứa khoảng trắng; nên đặt ngắn như A, B, C.
Trọng số là số nguyên từ 0 đến 9999, không dùng số âm.
Hai đầu cạnh phải khác nhau, đồ thị có tối đa 30 đỉnh.
File cần có ít nhất một cạnh hợp lệ; các đỉnh được tạo từ đầu cạnh.
Lưu file TXT dạng UTF-8 rồi dùng nút Insert để nhập.


7. LỖI THƯỜNG GẶP VÀ CÁCH SỬA

Không tìm thấy Qt6Config.cmake

Mở C:\msys64\ucrt64\lib\cmake\Qt6 và kiểm tra có Qt6Config.cmake.
Nếu chưa có, cài lại gói Qt ở phần “Cài compiler, Qt và các công cụ”.
Với CLion, mở CMake options theo phần “Mở CMake options và khai báo Qt” và kiểm tra đường dẫn là C:/msys64/ucrt64.
Với VS Code, kiểm tra biến CMAKE_PREFIX_PATH theo phần “Thêm công cụ vào PATH của Windows”, đóng hoàn toàn VS Code rồi mở lại và chạy CMake: Configure.

Không nhận lệnh g++, cmake hoặc ninja

Kiểm tra công cụ đã được cài ở phần “Cài compiler, Qt và các công cụ”.
Kiểm tra Path có C:\msys64\ucrt64\bin theo phần “Thêm công cụ vào PATH của Windows”.
Đóng terminal và IDE, mở lại rồi kiểm tra các lệnh ở phần “Kiểm tra trước khi mở project”.

Báo đường dẫn máy khác hoặc compiler không khớp cache

Không dùng lại cmake-build-debug đi kèm project.
Trong CLion, đổi Build directory thành một tên mới, chẳng hạn build-clion-ucrt64-new, rồi Reload CMake Project.
Trong VS Code, mở Settings theo phần “Chọn CMake và thư mục build bằng giao diện Settings”, đổi Build Directory thành ${workspaceFolder}/build-vscode-ucrt64-new rồi chạy CMake: Configure.

Build lỗi sau khi chọn compiler

Kiểm tra compiler đang chọn thuộc C:\msys64\ucrt64\bin.
Kiểm tra Qt cũng thuộc C:\msys64\ucrt64, không lấy Qt từ D:/QT6 hoặc bộ khác để ghép vào cấu hình này.
Dùng thư mục build mới theo phần “Báo đường dẫn máy khác hoặc compiler không khớp cache”, sau đó configure và build lại.

Mở EXE báo thiếu DLL

Kiểm tra Path có C:\msys64\ucrt64\bin rồi đóng và mở lại IDE.
Chạy lại bằng nút Run trong IDE sau khi build thành công.
Nếu vẫn lỗi, mở PowerShell tại thư mục gốc project và chạy lệnh triển khai Qt dưới đây. Với CLion, thay build-vscode-ucrt64 thành build-clion-ucrt64:

    C:\msys64\ucrt64\bin\windeployqt.exe --compiler-runtime .\build-vscode-ucrt64\source\DijkstraSimulator.exe

Nếu thiếu DLL khác thuộc MSYS2, kiểm tra gói tương ứng đã cài và dùng PATH của UCRT64 khi chạy. windeployqt có thể không sao chép hết mọi phụ thuộc MSYS2.
Khi gửi ứng dụng sang máy khác, cần gửi cả DLL và thư mục plugin cần thiết cùng EXE; kiểm tra trên máy nhận trước khi sử dụng.

Báo lỗi Qt platform plugin windows

Chạy lệnh triển khai Qt ở phần “Mở EXE báo thiếu DLL” với đúng đường dẫn EXE.
Kiểm tra thư mục cạnh EXE có platforms/qwindows.dll.
Kiểm tra Path ưu tiên C:\msys64\ucrt64\bin, không nạp Qt từ một bộ khác.

VS Code không thấy kit hoặc target

Nếu không thấy kit, kiểm tra g++ bằng phần “Kiểm tra trước khi mở project”, mở lại VS Code rồi chạy CMake: Scan for Kits.
Nếu không thấy target DijkstraSimulator, kiểm tra đã mở đúng thư mục gốc chứa CMakeLists.txt.
Chạy CMake: Configure. Nếu báo lỗi, bấm View, bấm Output và chọn kênh CMake hoặc CMake Tools để đọc lỗi đầu tiên.
Chỉ chọn target và chạy sau khi configure thành công.

Breakpoint không dừng

Kiểm tra đã chọn Debug thay vì Release.
Kiểm tra target đang chạy là DijkstraSimulator.
Đặt breakpoint ở dòng có thể thực thi và build lại.
Với CLion, kiểm tra đường dẫn Debugger ở phần “Chọn bộ công cụ MSYS2”. Với VS Code, kiểm tra GDB và PATH ở mục 2.

Không nhập được file TXT

Đọc số dòng được thông báo trong cửa sổ lỗi.
Mở file TXT và sửa dòng đó theo mục 6.
Kiểm tra trọng số không âm, hai đầu cạnh khác nhau và không quá 30 đỉnh.
Lưu file rồi bấm Insert để nhập lại.


8. KIỂM TRA TRƯỚC KHI SỬ DỤNG

Đã cài MSYS2 và các gói UCRT64 ở mục 2.
GCC, Qt, GDB, CMake và Ninja cùng thuộc bộ UCRT64.
Đã mở đúng thư mục gốc project.
Đã dùng thư mục build mới, tách riêng cho từng IDE.
Configure và build đều thành công.
Cửa sổ ứng dụng mở được.
Chạy thử do_thi_mau.txt cho kết quả a đến d bằng 30.


9. TÀI LIỆU THAM KHẢO

Tải và cài MSYS2: https://www.msys2.org/
CMake trong MSYS2: https://www.msys2.org/docs/cmake/
Gói Qt 6 UCRT64: https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-qt6-base
Hướng dẫn GCC và MSYS2 của Microsoft: https://code.visualstudio.com/docs/cpp/config-mingw
Hướng dẫn Qt trên CLion: https://www.jetbrains.com/help/clion/qt-tutorial.html

Hướng dẫn đã được đối chiếu với mã nguồn và tài liệu công cụ. Chưa kiểm thử trực tiếp toàn bộ quy trình cài MSYS2 mới và chạy giao diện trên cả hai IDE.

