# BTL_NetworkProgramming
Chat program base on MQTT protocol 
	1. Mô tả ngắn gọn chương trình
		Chương trình Chat xây dựng dựa trên giao thức kiểu publish/subscribe MQTT
	2. Môi trường chạy của chương trình
		- Hệ điều hành: Linux
		- Programming language: C
	3. Tính năng 
		Các chức năng chính:
			- Cho phép người dùng chat với nhau
			- Cho phép người dùng chat nhóm (group chat)
			- Cho phép người dùng upload File lên Server và download File từ Server
		Các yêu cầu lựa chọn:
			- Mời bạn vào nhóm chat
			- Tự join vào group chat
			- Yêu cầu hiển thị listFile, listuser, list các lệnh khác...
			
	4. Cách sử dụng
		1. Người dùng nhập tên đăng nhập (username)
		2. Người dùng tạo một nhóm chat (lệnh @create <room_Name>)
		3. Người dùng nhập lệnh @invite <username> để mời bạn vào room chat, @join để thêm mình vào room chat khác
			hoặc sử dụng lệnh @help để biết thêm chi tiết các lệnh dùng tiếp theo
		4. Khi có từ 2 người dùng trở lên ở trong room chat thì tính năng chat được kích hoạt, người dùng nhập các dòng
			chat để nói chuyện với các người dùng khác, sử dụng các lệnh lựa chọn có trong menu để đáp ứng các yêu cầu 
			khác nhau (@help)
		5. Sử dụng lệnh @out để thoát khỏi room chat và trở về main, lệnh @exit để ngắt kết nốt và out khỏi chương trình