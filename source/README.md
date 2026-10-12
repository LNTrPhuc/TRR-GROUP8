# Dijkstra Interactive — cập nhật 08/10/2026

Mở `app/update1.3.exe` trong thư mục gốc dự án để chạy bản mới nhất trên Windows 64-bit. Giữ file chạy cùng các thư viện và thư mục plugin Qt trong `app`.
Dự án có mã nguồn C++20 cho CLion, file TXT mẫu và báo cáo project gốc. Ảnh kiểm tra mới nhất nằm trong `build-updated/checks`.

## Thao tác đồ thị

- Chuột phải vào cạnh thuộc nhóm chọn: Xóa cạnh áp dụng cho mọi cạnh đang chọn; Xóa hướng áp dụng cho mọi cạnh có hướng trong nhóm, giữ trọng số và các cạnh vô hướng. Đỉnh đang chọn được giữ lại. Chuột phải vào cạnh ngoài nhóm chỉ tác động cạnh đó.
- Khi có hai cạnh có hướng ngược chiều, Xóa hướng chỉ đổi cạnh được chọn sang vô hướng; cạnh còn lại giữ nguyên chiều và trọng số. Các cạnh song song được vẽ tách nhau và có thể sửa/xóa riêng. Chuyển toàn đồ thị sang Vô hướng bỏ hướng từng cạnh, giữ riêng mọi cạnh cùng trọng số và định danh; không gộp cạnh.
- Nút phần trăm thu phóng hiển thị tỉ lệ hiện tại khi dùng con lăn hoặc nút −/+; bấm nút này đặt lại 100% và khung nhìn.
- Bấm chuột trái vào đỉnh để chọn: vòng đỏ bao quanh đỉnh. Chọn một đỉnh rồi nhấn/kéo trái lần tiếp theo sang đỉnh khác để nối cạnh; nhấn/thả tại chỗ giữ lựa chọn. Alt+kéo trái di chuyển đỉnh đã chọn; kéo một đỉnh trong nhóm vẫn di chuyển cả nhóm.
- Khi đỉnh/cạnh đang được chọn, bấm Backspace (hoặc Delete) để xóa lựa chọn và các cạnh liên quan. Nút Xóa đỉnh đã được bỏ.
- Bấm đúp chuột trái vào đỉnh để sửa tên ngay trên đỉnh. Chữ cập nhật trực tiếp trong ô trong suốt, có con trỏ và hỗ trợ nhập tiếng Việt.
- Enter hoặc bấm ra ngoài để xác nhận tên; Esc để hủy. Backspace trong lúc nhập tên dùng để xóa chữ. Tên rỗng/trùng không được chấp nhận và giữ tên cũ.
- Bấm chuột trái vào vùng trắng để bỏ chọn.
- Chuột phải vào đỉnh: Đặt làm điểm xuất phát / Đặt làm điểm kết thúc / Bỏ làm điểm kết thúc. Không còn mục đổi tên trong menu.
- Chọn Bỏ làm điểm kết thúc để chạy Dijkstra từ nguồn đến mọi đỉnh có thể tới được, không dừng sớm ở một đích. Khi hoàn tất, tô hồng cây đường đi ngắn nhất. Đỉnh không thể tới vẫn có khoảng cách ∞.
- Giữ chuột phải trên đỉnh, kéo sang đỉnh khác rồi thả để nối cạnh, trọng số ban đầu 1.
- Bấm đúp chuột trái vào trọng số để sửa ngay trên canvas. Enter hoặc bấm ra ngoài để lưu; Esc để hủy. Trọng số nguyên 0–9999.
- Chuột phải vào đường cạnh hoặc ô trọng số để sửa/xóa cạnh, kể cả khi cạnh đang được chọn. Cạnh có hướng luôn có mục Xóa hướng; nếu đồ thị đang có hướng thì xóa hướng riêng một cạnh chuyển chế độ sang hỗn hợp và giữ nguyên các cạnh khác. Trong chế độ hỗn hợp, cạnh vô hướng có mục Thêm hướng.
- Bộ chọn “Đồ thị: vô hướng / có hướng / hỗn hợp” định dạng lại toàn đồ thị hiện tại. Vô hướng: bỏ mũi tên, giữ mọi cạnh và trọng số riêng. Có hướng: cạnh vô hướng nhận chiều từ đỉnh có thứ tự tạo thấp hơn tới cao hơn; cạnh đã có hướng giữ chiều và trọng số. Hỗn hợp: giữ các cạnh hiện tại, cạnh mới mặc định có hướng và có thể xóa hướng qua menu trọng số. Các cạnh song song giữ trọng số độc lập.
- Hộp Tạo ngẫu nhiên có lựa chọn Loại đồ thị: Vô hướng hoặc Có hướng, độc lập với bố trí đỉnh. Chỉ áp dụng sau khi bấm Tạo đồ thị; Hủy giữ nguyên đồ thị và chế độ hiện tại.
- Ô sửa trọng số bấm đúp có nền trong suốt, con trỏ và màu chọn chữ giống ô sửa tên; Enter/bấm ngoài lưu, Esc hủy.
- Kéo chuột trái từ vùng trắng để tạo khung chọn nhiều đỉnh/cạnh. Ctrl hoặc Shift + bấm để thêm/bớt lựa chọn; Ctrl+A chọn tất cả; Esc bỏ chọn. Kéo một đỉnh trong nhóm để di chuyển cả nhóm đỉnh. Khung chọn lấy đỉnh có tâm nằm trong khung và cạnh cắt khung hoặc có nhãn trọng số trong khung.
- Lăn chuột để zoom tại con trỏ (20%–400%); nút −/+ để zoom tại tâm; nút 100% đặt lại zoom và vị trí khung nhìn. Kéo chuột giữa để dịch khung nhìn.
- Chọn Nguồn và Đích ở hàng phía trên canvas. Chọn Đích → Mọi đỉnh để tính tất cả khoảng cách và tô cây đường đi ngắn nhất; đỉnh không tới được giữ ∞.
- Chuột phải vào vùng trắng để thêm đỉnh.

## Nhập file TXT

Nút Insert mở hộp chọn file của Windows. Chọn file `.txt` có mỗi dòng gồm tên hai đỉnh và trọng số, ví dụ:

```text
a b 10
c d 20
a c 10
```

File mẫu: `examples/do_thi_mau.txt`.

Dòng đầu tiên của file có thể chọn bố trí đỉnh: `Grip` (hoặc `Grid`) = lưới, `Circle` = hình tròn, `Freedom` = tự do, `Square` = hình vuông. Nếu không có dòng này hoặc từ khóa không hợp lệ, bố trí mặc định là lưới. Dòng bắt đầu bằng `#` được xem là chú thích.

Insert cũng nhận dấu hướng giữa hai tên đỉnh:

```text
a -> b 10
b -> a 20
b c 5
```

`->` tạo cạnh đi từ tên bên trái sang tên bên phải; dòng `a b 10` tạo cạnh vô hướng trong mọi chế độ đồ thị. Hai dòng có hướng ngược chiều có trọng số độc lập. Các định dạng cũ `a -- b 10`, `a b 10 ->`, `a b 10 --` vẫn được hỗ trợ.

- Tên đỉnh không chứa khoảng trắng; giữ nguyên chữ hoa/thường và hỗ trợ UTF-8.
- Trọng số là số nguyên từ 0 đến 9999. Bỏ qua dòng trống. Ví dụ `a -> b 5` và `b c 2` để nhập đồ thị hỗn hợp: dòng không có dấu hướng luôn tạo cạnh vô hướng.
- Tối đa 30 tên đỉnh khác nhau. Vượt giới hạn sẽ báo "Đã quá số đỉnh cho phép (tối đa 30 đỉnh)".
- Không nhận cạnh tự nối. Nếu lặp lại một cạnh, trọng số ở dòng sau được dùng.
- Nhập thành công thay đồ thị hiện tại, lấy đỉnh xuất hiện đầu tiên làm nguồn và mặc định chạy không có điểm kết thúc. Có thể đặt đích qua menu chuột phải.
- Nhập thất bại hoặc hủy chọn file giữ nguyên đồ thị hiện tại.
- Tạo ngẫu nhiên hỗ trợ 2–30 đỉnh, các bố trí hình tròn, hình vuông, lưới và tự do. Bảng trắng chỉ nằm trong bộ chọn mẫu phía trên canvas, đã bỏ khỏi hộp Tạo ngẫu nhiên. Thêm đỉnh thủ công cũng giới hạn 30 đỉnh.

## Chạy thuật toán

- Kéo thanh tốc độ hết sang phải để tính kết quả tức thì, cập nhật bảng dữ liệu và tô đậm đường đi ngắn nhất. Các cạnh khác giữ màu nền nhạt, không tô xám đậm các tuyến tạm thời.
- Nếu thanh đang ở mức tối đa, bấm Chạy tự động hoặc Bước tiếp cũng tính kết quả tức thì.
- Tốc độ tăng theo cấp số nhân. Từ mức thấp tới sát mức tối đa, thời gian chạy toàn bài giảm từ khoảng 60 giây xuống 0,8 giây; mức mặc định khoảng 6 giây. Tốc độ tự cân bằng theo tổng số bước thực tế của bài toán, gồm số cạnh, nguồn và đích, nên đồ thị 30 đỉnh không bị chậm hơn chỉ vì nhiều bước.
- Ở tốc độ cao, mỗi khung hình chạy nhiều bước rồi cập nhật canvas/bảng một lần. Mọi bước vẫn được giữ để quay lại. Bước tiếp thủ công vẫn tiến đúng một bước, trừ mức tối đa tính tức thì.
- Cửa sổ mở theo vùng màn hình khả dụng và tỷ lệ DPI của Windows. Trên màn hình nhỏ, thanh cuộn cho phép tiếp cận đầy đủ bảng dữ liệu và nút điều khiển.
- Nối lại cùng hai đỉnh tạo thêm cạnh song song độc lập: hỗ trợ nhiều cạnh vô hướng hoặc nhiều cạnh có hướng cùng chiều. Trong chế độ hỗn hợp, vẽ đè lên cạnh vô hướng vẫn thêm hướng và giữ trọng số khi chưa có cạnh có hướng cùng chiều.
- Space để chạy/dừng; ←/→ để lùi/tiến. Trong khi đổi tên hoặc nhập trọng số, các phím dùng để nhập và di chuyển con trỏ.
- Đã bỏ dòng phụ "Khám phá đường đi ngắn nhất, từng bước một".

## Mở và build trong CLion

Mở thư mục `source` chứa `CMakeLists.txt`. Chọn Qt 6 Widgets và bộ MinGW tương thích. Trên máy này:

```text
-DCMAKE_PREFIX_PATH=D:/QT6/6.11.2/mingw_64
```

Dùng MinGW tại `D:/QT6/Tools/mingw1310_64`. CMake tự gọi windeployqt sau khi build. Trên máy khác, thay đường dẫn Qt/bộ biên dịch tương ứng.

Target `DijkstraSmoke` kiểm tra thuật toán, nhập TXT, giới hạn đỉnh, thao tác chuột/bàn phím, đổi tên trực tiếp, kết quả tức thì và bố trí giao diện. Chạy qua CTest sau khi build.

Chi tiết kết quả kiểm tra trong `KIEM_TRA.txt`.

Build kiểm chứng ở `build-updated`. Thư mục `build` cũ đã được bỏ vì cache thuộc đường dẫn khác.
Chạy `cmake --build build-updated` và `ctest --test-dir build-updated --output-on-failure` từ thư mục dự án.

## Kết quả từ nguồn tới mọi đỉnh và xem đường đi

- Chế độ Đích → Mọi đỉnh chạy cho tới khi hàng đợi trống, không dừng ở một đỉnh đích. Kết quả ghi số đỉnh tới được và số đỉnh không có đường từ nguồn theo chiều cạnh. Đỉnh không tới được giữ ∞; không đánh dấu chốt một khoảng cách hữu hạn giả.
- Đồ thị ngẫu nhiên có hướng đã sửa cây nối để nguồn ban đầu (đỉnh A) đi tới tất cả các đỉnh, kể cả mật độ cạnh thêm 0%. Đổi nguồn, xóa cạnh hoặc đổi chiều có thể làm một số đỉnh không tới được.
- Khi Đích là Mọi đỉnh, rê chuột lên bất kỳ ô trong một dòng bảng dữ liệu để xem bảng đồ họa riêng của tuyến từ nguồn tới đỉnh đó: các đỉnh, cạnh đúng chiều, trọng số và tổng chi phí. Tuyến dài tự xuống hàng. Rời bảng hoặc chọn đích cụ thể sẽ ẩn bảng xem.
- Trong lúc chạy, tuyến chưa được chốt ghi Đường đi tạm thời và cập nhật theo từng bước. Đỉnh đã chốt hiển thị đường đi ngắn nhất. Nguồn có khoảng cách 0; đỉnh không tới được có thông báo riêng. Xem tuyến không thay đổi nguồn, đích hay trạng thái mô phỏng.
