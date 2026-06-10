# Survival Simulator AI

> Môi trường mô phỏng sinh tồn 2D sử dụng Reinforcement Learning (PPO) để nghiên cứu hành vi sinh tồn nổi lên (Emergent Survival Behavior).

---

## Giới thiệu

**Survival Simulator AI** là một môi trường mô phỏng sinh tồn được xây dựng bằng **C++** và **SFML** 3.0, trong đó một tác tử AI (Agent) phải học cách sống sót trong môi trường liên tục thay đổi.

Thay vì được lập trình sẵn các hành vi như:

* Đi tìm thức ăn
* Chạy trốn kẻ thù
* Tấn công mục tiêu yếu hơn

Agent chỉ nhận được trạng thái môi trường và phần thưởng (Reward). Từ đó, nó phải tự khám phá và học ra chiến lược sinh tồn tối ưu.

Dự án được xây dựng nhằm nghiên cứu khả năng hình thành các hành vi sinh tồn phức tạp thông qua **Reinforcement Learning**.

---

# Bài toán

Trong môi trường tồn tại ba thực thể chính:

## Player (Agent)

Đây là tác tử được điều khiển bởi thuật toán PPO.

Agent cần:

* Duy trì HP
* Duy trì mức Hunger
* Gia tăng Power
* Sống sót càng lâu càng tốt

---

## Food

Food xuất hiện ngẫu nhiên trên bản đồ.

Khi ăn Food:

* Hunger được hồi phục
* Power được tăng

Food đóng vai trò là nguồn tài nguyên thiết yếu giúp Agent tồn tại.

---

## Enemy

Mỗi Enemy sở hữu một chỉ số Power riêng.

### Nếu:

Player Power ≥ Enemy Power

Agent sẽ:

* Tiêu diệt Enemy
* Nhận thêm Power

### Nếu:

Player Power < Enemy Power

Agent sẽ:

* Nhận sát thương
* Có nguy cơ tử vong

Điều này buộc Agent phải học cách đánh giá rủi ro trước khi giao chiến.

---

# Mục tiêu học của Agent

Một Agent thành công cần học được các hành vi:

✅ Tìm kiếm thức ăn khi đói

✅ Ghi nhớ hướng xuất hiện của thức ăn

✅ Tránh xa kẻ địch mạnh hơn

✅ Chủ động săn đuổi mục tiêu yếu hơn

✅ Khám phá môi trường

✅ Không mắc kẹt tại góc hoặc biên bản đồ

✅ Tối đa hóa số steps của mỗi Episode

---

# Môi trường mô phỏng

| Thuộc tính           | Giá trị     |
| -------------------- | ----------- |
| Kích thước bản đồ    | 1920 × 1080 |
| Số lượng Food        | 25          |
| Số lượng Enemy       | 12          |
| Respawn Food         | 3 giây      |
| Respawn Enemy        | 8 giây      |
| Chế độ học           | PPO         |
| Không gian hành động | 8 hướng     |
| Tốc độ khung hình    | 144 FPS      |

---

# Quan sát của AI (State Space)

Agent không được nhìn thấy toàn bộ bản đồ.

Nó chỉ quan sát môi trường thông qua các đặc trưng sau.

## Trạng thái bản thân

* HP Ratio
* Hunger Ratio
* Current Power

---

## Thức ăn gần nhất

* Có nhìn thấy hay không
* Hướng đến Food
* Khoảng cách đến Food
* Hướng Food cuối cùng được quan sát

---

## Kẻ địch

### Enemy gần nhất

* Khoảng cách
* Hướng
* Tỷ lệ sức mạnh

### Enemy nguy hiểm nhất

* Khoảng cách
* Hướng
* Tỷ lệ sức mạnh

---

## Cảm biến môi trường

Agent được trang bị 4 cảm biến khoảng cách tới:

* Tường trái
* Tường phải
* Tường trên
* Tường dưới

Nhờ đó Agent có thể học cách tránh bị kẹt vào góc.

---

# Không gian hành động (Action Space)

Agent có thể thực hiện 8 hành động:

| Action | Hướng |
| ------ | ----- |
| 0      | ↑     |
| 1      | ↗     |
| 2      | →     |
| 3      | ↘     |
| 4      | ↓     |
| 5      | ↙     |
| 6      | ←     |
| 7      | ↖     |

---

# Thiết kế Reward

Reward là yếu tố quyết định Agent học được hành vi gì.

## Reward dương

| Hành vi                 | Mục đích                         |
| ----------------------- | -------------------------------- |
| Ăn Food                 | Khuyến khích tìm kiếm tài nguyên |
| Tiêu diệt Enemy yếu hơn | Khuyến khích săn mồi             |
| Tiến gần Food           | Tăng khả năng khám phá           |
| Chạy xa Enemy mạnh      | Học cách sinh tồn                |
| Sống lâu hơn            | Tối đa hóa tuổi thọ              |

---

## Reward âm

| Hành vi             | Mục đích                   |
| ------------------- | -------------------------- |
| Bị tấn công         | Tránh giao chiến nguy hiểm |
| Đánh Enemy mạnh hơn | Học đánh giá rủi ro        |
| Chết                | Hình phạt lớn              |
| Lao vào tường       | Tránh hành vi vô nghĩa     |
| Kẹt góc bản đồ      | Chống exploit              |

---

# Chống kẹt biên và góc

Một vấn đề phổ biến của Reinforcement Learning là Agent tìm ra các hành vi không mong muốn.

Trong quá trình huấn luyện, Agent thường:

* Chạy dọc biên
* Kẹt trong góc
* Lặp lại hành động vô nghĩa

Hệ thống bổ sung cơ chế:

* Phạt khi tiếp tục di chuyển về phía tường
* Phạt mạnh hơn khi mắc kẹt tại góc
* Khuyến khích thoát khỏi vùng nguy hiểm

Điều này giúp Agent học được khả năng điều hướng tự nhiên hơn.

---

# Tăng độ khó động

Để tránh hiện tượng Agent trở nên quá mạnh và thống trị môi trường:

Power của Enemy sẽ tự động tăng khi Player phát triển.

Điều này tạo ra một môi trường học liên tục và duy trì áp lực sinh tồn trong suốt quá trình huấn luyện.

---

# Chế độ hoạt động

## AI Training Mode

```cpp
static constexpr bool AI_MODE = true;
```

Agent được điều khiển hoàn toàn bởi PPO.

---

## Manual Testing Mode

```cpp
static constexpr bool AI_MODE = false;
```

Người chơi điều khiển trực tiếp để kiểm thử gameplay.

---

# Cách tải 

* Tải từng file hoặc tải phiên bản release đã đươc up lên repo
* Mở file SurvivalGame.sln bằng Visual Studio 2022 trở lên
* Chỉnh chế độ Configuration thành Release sau đó tiến hành chạy game

# Lưu và tải mô hình

| Phím | Chức năng    |
| ---- | ------------ |
| F5   | Lưu trọng số |
| F9   | Tải trọng số |
| ESC  | Thoát        |

Model được lưu dưới dạng:

```text
player_weights.pt
```

---

# Công nghệ sử dụng

* C++17
* SFML
* Reinforcement Learning
* PPO (Proximal Policy Optimization)
* LibTorch

---

# Kết quả mong đợi

Sau một số lượng Episode đủ lớn, Agent có thể hình thành các hành vi:

* Tự tìm kiếm thức ăn
* Tránh các mục tiêu nguy hiểm
* Chủ động săn Enemy yếu hơn
* Khai thác tài nguyên hiệu quả
* Duy trì sự sống trong thời gian dài

Mà không cần được lập trình trực tiếp các quy tắc này.

---

# License

MIT License

Tự do sử dụng, chỉnh sửa và mở rộng cho mục đích học tập, nghiên cứu và phát triển.
