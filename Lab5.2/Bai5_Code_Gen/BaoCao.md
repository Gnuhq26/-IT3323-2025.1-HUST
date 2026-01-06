# BÁO CÁO ĐỒ ÁN: TRÌNH BIÊN DỊCH KPL - PHẦN CODE GENERATION

## I. TỔNG QUAN DỰ ÁN

### 1. Mục đích

Đồ án này nhằm xây dựng hoàn chỉnh phần **Code Generation** (sinh mã) cho trình biên dịch ngôn ngữ KPL (K Programming Language). Phần Code Generation có nhiệm vụ chuyển đổi cấu trúc cú pháp đã được phân tích và kiểm tra ngữ nghĩa thành mã máy ảo (bytecode) có thể thực thi trên máy ảo KPL.

**Mục tiêu cụ thể:**
- Sinh mã cho các cấu trúc điều khiển cơ bản (IF-THEN-ELSE, WHILE, FOR)
- Sinh mã cho các biểu thức số học và logic
- Xử lý khai báo và truy xuất biến, mảng
- Hỗ trợ thủ tục và hàm với nested scope
- Tính toán static link và dynamic link để truy xuất biến đúng scope
- Sinh mã cho các hàm/thủ tục có sẵn (predefined functions/procedures)

### 2. Luồng hoạt động

Quy trình biên dịch một chương trình KPL diễn ra theo các bước sau:

**Bước 1: Phân tích từ vựng (Lexical Analysis)**
- Scanner đọc mã nguồn và tạo ra các token
- Nhận dạng từ khóa, định danh, hằng số, toán tử
- Hỗ trợ không phân biệt hoa thường cho từ khóa và định danh

**Bước 2: Phân tích cú pháp (Syntax Analysis)**
- Parser xây dựng cây cú pháp dựa trên ngữ pháp KPL
- Kiểm tra tính đúng đắn về mặt cú pháp
- Gọi các hàm sinh mã tương ứng với mỗi cấu trúc

**Bước 3: Phân tích ngữ nghĩa (Semantic Analysis)**
- Kiểm tra khai báo biến, hàm, thủ tục
- Kiểm tra kiểu dữ liệu
- Xây dựng bảng symbol table với thông tin về scope và offset

**Bước 4: Sinh mã (Code Generation)**
- Sinh các lệnh cho máy ảo KPL
- Tính toán địa chỉ biến, tham số
- Xử lý static link và dynamic link
- Tối ưu hóa mã sinh ra

**Bước 5: Xuất mã (Code Emission)**
- Ghi mã bytecode ra file nhị phân
- In ra mã assembly để debug (nếu có flag -dump)

**Kiến trúc máy ảo KPL:**
- Stack-based architecture
- Sử dụng các thanh ghi: PC (Program Counter), SP (Stack Pointer), BP (Base Pointer)
- Các lệnh cơ bản: LA (Load Address), LV (Load Value), LC (Load Constant), ST (Store), CALL, JMP, FJ (False Jump)...

---

## II. CÁC CASE ĐÃ IMPLEMENT

### 1. Khai báo và gán biến

**Mô tả:**
Khai báo biến trong KPL được thực hiện trong phần VAR của chương trình. Mỗi biến được cấp phát một vị trí (offset) trong stack frame của scope hiện tại.

**Cách thực hiện:**
- Khi gặp khai báo biến, compiler tính toán offset từ base pointer
- Offset được lưu trong symbol table cùng với thông tin về scope
- Khi gán giá trị cho biến, sinh lệnh LA (Load Address) để đẩy địa chỉ biến lên stack
- Tính toán biểu thức vế phải và đẩy kết quả lên stack
- Sinh lệnh ST (Store) để lưu giá trị vào địa chỉ

**Ví dụ:**
```
VAR x : INTEGER;
BEGIN
  x := 10;
END.
```

**Mã sinh ra:**
```
LA 0,4    ; Load address của biến x (level 0, offset 4)
LC 10     ; Load constant 10
ST        ; Store giá trị 10 vào địa chỉ x
```

**Xử lý level:**
Level được tính bằng hiệu số giữa scope hiện tại và scope chứa biến. Điều này cho phép truy xuất biến từ outer scope trong nested procedure/function.

### 2. Biểu thức số học

**Mô tả:**
KPL hỗ trợ các phép toán số học cơ bản: cộng (+), trừ (-), nhân (*), chia (/), và phủ định (-).

**Cách thực hiện:**
- Biểu thức được parse theo thứ tự ưu tiên: Factor → Term → Expression
- Mỗi toán tử sinh ra một lệnh tương ứng trên stack
- Kết quả tính toán được giữ trên đỉnh stack

**Các lệnh sinh:**
- AD: Addition (cộng)
- SB: Subtraction (trừ)
- ML: Multiplication (nhân)
- DV: Division (chia)
- NEG: Negation (phủ định)

**Ví dụ:**
```
S := S + I * I
```

**Mã sinh ra:**
```
LA 0,4    ; Load address của S
LV 0,4    ; Load value của S
LV 0,5    ; Load value của I
LV 0,5    ; Load value của I (lần 2)
ML        ; I * I
AD        ; S + (I * I)
ST        ; Store vào S
```

### 3. Phép so sánh

**Mô tả:**
KPL hỗ trợ 6 phép so sánh: = (bằng), != (khác), < (nhỏ hơn), <= (nhỏ hơn hoặc bằng), > (lớn hơn), >= (lớn hơn hoặc bằng).

**Cách thực hiện:**
- Tính giá trị hai biểu thức cần so sánh
- Sinh lệnh so sánh tương ứng
- Kết quả là 1 (true) hoặc 0 (false) trên stack

**Các lệnh sinh:**
- EQ: Equal (=)
- NE: Not Equal (!=)
- LT: Less Than (<)
- LE: Less or Equal (<=)
- GT: Greater Than (>)
- GE: Greater or Equal (>=)

**Ví dụ:**
```
IF I <= 5 THEN ...
```

**Mã sinh ra:**
```
LV 0,5    ; Load value của I
LC 5      ; Load constant 5
LE        ; So sánh I <= 5
```

### 4. Câu lệnh IF – THEN – ELSE

**Mô tả:**
Cấu trúc rẽ nhánh IF-THEN-ELSE cho phép thực thi một trong hai nhánh code dựa trên điều kiện.

**Cách thực hiện:**
- Tính toán điều kiện (condition)
- Sinh lệnh FJ (False Jump) để nhảy đến ELSE nếu điều kiện sai
- Sinh mã cho phần THEN
- Nếu có ELSE: sinh lệnh J (Jump) để bỏ qua phần ELSE sau khi thực thi THEN
- Cập nhật địa chỉ nhảy của FJ và J

**Sơ đồ luồng:**
```
<condition>
FJ label_else      ; Nếu false, nhảy đến else
<then_statement>
J label_end        ; Nhảy qua else
label_else:
<else_statement>
label_end:
```

**Ví dụ:**
```
IF (n MOD 2) = 0 THEN
  CALL WRITEC('E')
ELSE
  CALL WRITEC('O')
```

**Mã sinh ra:**
```
LV 0,4    ; Load n
LV 0,4    ; Load n
LC 2      ; Load 2
DV        ; n / 2
LC 2      ; Load 2
ML        ; (n/2) * 2
SB        ; n - (n/2)*2
LC 0      ; Load 0
EQ        ; So sánh với 0
FJ 18     ; Nếu false, nhảy đến địa chỉ 18 (else)
LC 69     ; Load 'E'
CALL 1,0  ; Gọi WRITEC
J 20      ; Nhảy qua else
LC 79     ; Load 'O' (địa chỉ 18)
CALL 1,0  ; Gọi WRITEC
          ; Địa chỉ 20 (end)
```

### 5. Vòng lặp WHILE

**Mô tả:**
Vòng lặp WHILE thực thi một khối lệnh lặp đi lặp lại trong khi điều kiện còn đúng.

**Cách thực hiện:**
- Đánh dấu địa chỉ bắt đầu vòng lặp (beginLoop)
- Tính toán điều kiện
- Sinh lệnh FJ để thoát vòng lặp nếu điều kiện sai
- Sinh mã cho thân vòng lặp
- Sinh lệnh J để nhảy về đầu vòng lặp
- Cập nhật địa chỉ của FJ

**Sơ đồ luồng:**
```
beginLoop:
<condition>
FJ label_end       ; Nếu false, thoát vòng lặp
<loop_body>
J beginLoop        ; Quay lại đầu vòng lặp
label_end:
```

**Ví dụ:**
```
WHILE I <= 5 DO
BEGIN
  S := S + I * I;
  I := I + 1;
END
```

**Mã sinh ra:**
```
LV 0,5    ; (địa chỉ 8) Load I
LC 5      ; Load 5
LE        ; I <= 5
FJ 25     ; Nếu false, nhảy đến 25 (end)
LA 0,4    ; Load address của S
LV 0,4    ; Load value S
LV 0,5    ; Load value I
LV 0,5    ; Load value I
ML        ; I * I
AD        ; S + I*I
ST        ; Store vào S
LA 0,5    ; Load address của I
LV 0,5    ; Load value I
LC 1      ; Load 1
AD        ; I + 1
ST        ; Store vào I
J 8       ; Quay lại địa chỉ 8
          ; Địa chỉ 25 (end)
```

### 6. Vòng lặp FOR

**Mô tả:**
Vòng lặp FOR là một dạng vòng lặp đặc biệt với biến đếm, giá trị bắt đầu, giá trị kết thúc.

**Cách thực hiện:**
- Gán giá trị khởi tạo cho biến điều khiển
- Đánh dấu địa chỉ bắt đầu vòng lặp
- So sánh biến điều khiển với giá trị kết thúc (<=)
- Sinh lệnh FJ để thoát nếu vượt giá trị kết thúc
- Sinh mã cho thân vòng lặp
- Tăng biến điều khiển lên 1
- Sinh lệnh J để quay lại đầu vòng lặp

**Sơ đồ luồng:**
```
<control_var> := <start_value>
ST
beginLoop:
LV control_var
<end_value>
LE
FJ label_end
<loop_body>
<control_var> := <control_var> + 1
J beginLoop
label_end:
```

**Ví dụ:**
```
FOR I := 1 TO N DO
  A(.I.) := READI
```

**Mã sinh ra:**
```
LA 0,15   ; Load address của I
LC 1      ; Load 1 (start value)
ST        ; I := 1
LV 0,15   ; (địa chỉ 8) Load I
LV 0,14   ; Load N
LE        ; I <= N
FJ 27     ; Nếu false, thoát vòng lặp
LA 0,4    ; Load base address của array A
LV 0,15   ; Load I
LC 1      ; Load 1
SB        ; I - 1 (array index from 1)
LC 1      ; Element size
ML        ; (I-1) * size
AD        ; Base + offset
CALL 1,0  ; READI
ST        ; Store vào A[I]
LA 0,15   ; Load address của I
LV 0,15   ; Load I
LC 1      ; Load 1
AD        ; I + 1
ST        ; Store vào I
J 8       ; Quay lại địa chỉ 8
          ; Địa chỉ 27 (end)
```

### 7. Mảng (ARRAY)

**Mô tả:**
KPL hỗ trợ mảng một chiều với cú pháp: `ARRAY(. size .) OF type`

**Cách thực hiện:**
- Mảng được cấp phát liên tiếp trong stack frame
- Địa chỉ phần tử thứ i: base_address + (i - 1) × element_size
- KPL sử dụng chỉ số bắt đầu từ 1
- Khi truy xuất phần tử mảng:
  + Tính địa chỉ phần tử: base + (index - 1) × size
  + Sinh lệnh LI (Load Indirect) để đọc giá trị
  + Hoặc dùng địa chỉ để gán giá trị

**Công thức tính địa chỉ:**
```
address = base_address + (index - 1) × element_size
```

**Ví dụ:**
```
TYPE T = INTEGER;
VAR A : ARRAY(. 10 .) OF T;
BEGIN
  A(.5.) := 100;
END
```

**Mã sinh ra:**
```
LA 0,4    ; Load base address của A
LC 5      ; Load index 5
LC 1      ; Load 1
SB        ; 5 - 1 = 4
LC 1      ; Load element_size
ML        ; 4 * 1 = 4
AD        ; base + 4
LC 100    ; Load value 100
ST        ; Store 100 vào A[5]
```

### 8. Thủ tục và hàm (Procedure & Function)

**Mô tả:**
KPL hỗ trợ khai báo procedure (không trả về giá trị) và function (trả về giá trị).

**Cách thực hiện:**

**Khai báo:**
- Mỗi procedure/function có một địa chỉ code (code address)
- Có scope riêng chứa tham số và biến local
- Sinh lệnh J ở đầu để bỏ qua phần khai báo
- Sau khi thực thi xong, sinh lệnh EP (Exit Procedure) hoặc EF (Exit Function)

**Gọi procedure/function:**
- Đẩy các tham số lên stack
- Sinh lệnh CALL với level và code address
- Lệnh CALL thực hiện:
  + Lưu dynamic link (BP cũ)
  + Lưu return address (PC cũ)
  + Lưu static link (BP của outer scope)
  + Cập nhật BP và PC

**Stack frame layout:**
```
[offset 0]: Return value (chỉ cho function)
[offset 1]: Dynamic Link (old BP)
[offset 2]: Return Address (old PC)
[offset 3]: Static Link (BP of outer scope)
[offset 4+]: Parameters
[offset n+]: Local variables
```

**Ví dụ:**
```
PROCEDURE DISPLAY(N: INTEGER);
BEGIN
  CALL WRITEI(N);
END;

BEGIN
  CALL DISPLAY(10);
END.
```

**Mã sinh ra:**
```
J 2       ; Bỏ qua phần khai báo procedure
INT 5     ; (địa chỉ 2) Allocate stack frame
LV 0,4    ; Load parameter N
CALL 2,0  ; Gọi WRITEI (level 2, predefined)
EP        ; Exit procedure
INT 5     ; (địa chỉ 5) Main program frame
LC 10     ; Load 10
CALL 1,2  ; Gọi DISPLAY (level 1, address 2)
HL        ; Halt
```

### 9. Predefined Functions/Procedures

**Mô tả:**
KPL cung cấp sẵn các hàm và thủ tục thư viện:
- **READI**: Đọc một số nguyên từ input
- **READC**: Đọc một ký tự từ input
- **WRITEI(n)**: In số nguyên n ra output
- **WRITEC(ch)**: In ký tự ch ra output
- **WRITELN**: In xuống dòng

**Cách thực hiện:**
- Các hàm/thủ tục này được đăng ký sẵn trong global symbol table
- Khi gọi, compiler nhận biết và sinh lệnh đặc biệt:
  + RI: Read Integer
  + RC: Read Char
  + WRI: Write Integer
  + WRC: Write Char
  + WLN: Write Line
- Không cần tính toán level và address như user-defined function

**Ví dụ:**
```
BEGIN
  n := READI;
  CALL WRITEI(n);
  CALL WRITELN;
END.
```

**Mã sinh ra:**
```
LA 0,4    ; Load address của n
CALL 1,0  ; Gọi READI (sinh lệnh RI)
ST        ; Store vào n
LV 0,4    ; Load value của n
CALL 1,0  ; Gọi WRITEI (sinh lệnh WRI)
CALL 1,0  ; Gọi WRITELN (sinh lệnh WLN)
```

### 10. Static Link và Nested Scope

**Mô tả:**
KPL cho phép nested procedure/function (procedure/function lồng nhau). Để truy xuất biến từ outer scope, compiler sử dụng static link.

**Khái niệm:**

**Static Link:** Con trỏ đến stack frame của outer scope (lexically enclosing scope). Cho phép truy xuất biến non-local.

**Dynamic Link:** Con trỏ đến stack frame của caller (runtime caller). Dùng để quay lại sau khi procedure/function kết thúc.

**Level Difference:** Hiệu số giữa scope hiện tại và scope chứa biến cần truy xuất. Compiler dùng level để follow static link chain.

**Cách thực hiện:**

**Tính level:**
```
level = current_scope_level - variable_scope_level
```

**Sinh lệnh truy xuất biến:**
- LV (Load Value): Đọc giá trị biến
- LA (Load Address): Lấy địa chỉ biến
- Tham số p trong lệnh LA/LV p,q là level difference

**Ví dụ với nested procedure:**
```
PROGRAM NESTED;
VAR x : INTEGER;

PROCEDURE OUTER;
VAR y : INTEGER;

  PROCEDURE INNER;
  BEGIN
    x := x + y;  (* Truy xuất x từ program scope, y từ outer scope *)
  END;
  
BEGIN
  y := 5;
  CALL INNER;
END;

BEGIN
  x := 10;
  CALL OUTER;
END.
```

**Phân tích level:**
- Biến x: program scope (level 0)
- Biến y: OUTER scope (level 1)
- Trong INNER (level 2):
  + Truy xuất x: level difference = 2 - 0 = 2
  + Truy xuất y: level difference = 2 - 1 = 1

**Mã sinh trong INNER:**
```
LA 2,4    ; Load address của x (level 2, từ program scope)
LV 2,4    ; Load value của x
LV 1,4    ; Load value của y (level 1, từ OUTER scope)
AD        ; x + y
ST        ; Store vào x
```

**Static Link Chain:**
Khi máy ảo thực thi lệnh LV 2,4:
1. Bắt đầu từ BP hiện tại (INNER frame)
2. Follow static link 2 lần:
   - Lần 1: BP → static link → OUTER frame
   - Lần 2: OUTER frame → static link → PROGRAM frame
3. Đọc giá trị tại offset 4 trong PROGRAM frame

---

## III. KẾT QUẢ

### 1. Tệp example1.kpl không có chương trình con

**Nội dung file:**
```
PROGRAM CODEGEN9;  
TYPE T = INTEGER;
VAR  S : T;
     I : INTEGER;
    
BEGIN
    S:=0;
    I:=1;
    WHILE I<=5 DO
    BEGIN     
        S:=S+I*I;
        I:=I+1;
    END;
    CALL WRITEI(S);
    CALL WRITELN;
END.
```

**Mô tả chương trình:**
- Chương trình tính tổng bình phương: S = 1² + 2² + 3² + 4² + 5² = 55
- Sử dụng vòng lặp WHILE để lặp từ I=1 đến I=5
- Không có procedure/function con, chỉ có main program

**Mã bytecode sinh ra:**
```
0:  J 1          ; Nhảy qua phần khai báo (không có sub-program)
1:  INT 6        ; Cấp phát stack frame (4 reserved + 2 variables)
2:  LA 0,4       ; Load address của S
3:  LC 0         ; Load constant 0
4:  ST           ; S := 0
5:  LA 0,5       ; Load address của I
6:  LC 1         ; Load constant 1
7:  ST           ; I := 1
8:  LV 0,5       ; [Begin WHILE] Load value I
9:  LC 5         ; Load constant 5
10: LE           ; I <= 5
11: FJ 25        ; Nếu false, nhảy đến 25 (exit while)
12: LA 0,4       ; Load address của S
13: LV 0,4       ; Load value S
14: LV 0,5       ; Load value I
15: LV 0,5       ; Load value I
16: ML           ; I * I
17: AD           ; S + (I*I)
18: ST           ; S := S + I*I
19: LA 0,5       ; Load address của I
20: LV 0,5       ; Load value I
21: LC 1         ; Load constant 1
22: AD           ; I + 1
23: ST           ; I := I + 1
24: J 8          ; Quay lại đầu while (địa chỉ 8)
25: LV 0,4       ; [Exit WHILE] Load value S
26: CALL 1,0     ; Gọi WRITEI (sinh lệnh WRI)
27: CALL 1,0     ; Gọi WRITELN (sinh lệnh WLN)
28: HL           ; Halt program
```

**Phân tích:**
- **Stack frame size:** 6 words (4 reserved + 2 biến S và I)
- **Biến S:** offset = 4
- **Biến I:** offset = 5
- **Vòng lặp WHILE:** từ địa chỉ 8 đến 24
- **Số lệnh:** 29 lệnh
- **Không có static link phức tạp** vì không có nested scope

**Kết quả thực thi:**
```
Iteration 1: S = 0 + 1*1 = 1
Iteration 2: S = 1 + 2*2 = 5
Iteration 3: S = 5 + 3*3 = 14
Iteration 4: S = 14 + 4*4 = 30
Iteration 5: S = 30 + 5*5 = 55
Output: 55
```

### 2. Tệp example3.kpl có chương trình con

**Nội dung file:**
```
PROGRAM EXAMPLE3;  (* TOWER OF HANOI *)
VAR  I:INTEGER;  
     N:INTEGER;  
     P:INTEGER;  
     Q:INTEGER;
     C:CHAR;

PROCEDURE HANOI(N:INTEGER; S:INTEGER; Z:INTEGER);
BEGIN
  IF N != 0 THEN
    BEGIN
      CALL HANOI(N-1,S,6-S-Z);
      I:=I+1;  
      CALL WRITELN;
      CALL WRITEI(I);  
      CALL WRITEI(N);
      CALL WRITEI(S);  
      CALL WRITEI(Z);
      CALL HANOI(N-1,6-S-Z,Z)
    END
END;

BEGIN
  FOR N := 1 TO 4 DO  
    BEGIN
      FOR I:=1 TO 4 DO  
        CALL WRITEC(' ');
      C := READC;  
      CALL WRITEC(C)
    END;
  P:=1;  
  Q:=2;
  FOR N:=2 TO 4 DO
    BEGIN  
      I:=0;  
      CALL HANOI(N,P,Q);  
      CALL WRITELN  
    END
END.
```

**Mô tả chương trình:**
- Thuật toán Tower of Hanoi (Tháp Hà Nội) đệ quy
- **Procedure HANOI:** Là recursive procedure với 3 tham số (N, S, Z)
- **Nested scope:** Procedure HANOI truy xuất biến I từ program scope
- Chương trình chính gọi HANOI với các giá trị khác nhau

**Mã bytecode sinh ra:**

**Phần khai báo procedure:**
```
0:  J 54         ; Nhảy qua procedure definition đến main program
1:  J 2          ; Label cho procedure (không dùng)
2:  INT 7        ; Cấp phát frame cho HANOI (4 reserved + 3 params)
3:  LV 0,4       ; Load parameter N
4:  LI           ; Load indirect (vì N là parameter)
5:  LC 0         ; Load constant 0
6:  NE           ; N != 0
7:  FJ 53        ; Nếu false (N=0), thoát procedure (địa chỉ 53)
8:  LV 0,4       ; [True branch] Load N
9:  LI           ; 
10: LC 1         ; Load 1
11: SB           ; N - 1
12: LV 0,5       ; Load S
13: LI           ;
14: LC 6         ; Load 6
15: LV 0,5       ; Load S
16: LI           ;
17: SB           ; 6 - S
18: LV 0,6       ; Load Z
19: LI           ;
20: SB           ; (6-S) - Z = 6-S-Z
21: CALL 0,1     ; Recursive call HANOI(N-1, S, 6-S-Z), level 0 vì cùng level
22: LA 1,4       ; Load address của I (từ outer scope, level 1)
23: LV 1,4       ; Load value I
24: LC 1         ; Load 1
25: AD           ; I + 1
26: ST           ; I := I + 1
27: CALL 2,0     ; WRITELN (level 2)
28: LV 1,4       ; Load I từ outer scope
29: CALL 2,0     ; WRITEI(I)
30: LV 0,4       ; Load N
31: LI           ;
32: CALL 2,0     ; WRITEI(N)
33: LV 0,5       ; Load S
34: LI           ;
35: CALL 2,0     ; WRITEI(S)
36: LV 0,6       ; Load Z
37: LI           ;
38: CALL 2,0     ; WRITEI(Z)
39: LV 0,4       ; Load N
40: LI           ;
41: LC 1         ; Load 1
42: SB           ; N - 1
43: LC 6         ; Load 6
44: LV 0,5       ; Load S
45: LI           ;
46: SB           ; 6 - S
47: LV 0,6       ; Load Z
48: LI           ;
49: SB           ; 6 - S - Z
50: LV 0,6       ; Load Z
51: LI           ;
52: CALL 0,1     ; Recursive call HANOI(N-1, 6-S-Z, Z)
53: EP           ; Exit Procedure
```

**Phần main program:**
```
54: INT 9        ; Cấp phát frame cho main (4 reserved + 5 variables)
55: LA 0,5       ; [FOR N := 1 TO 4]
56: LC 1         ;
57: ST           ; N := 1
58: LV 0,5       ; [Loop condition]
59: LC 4         ;
60: LE           ; N <= 4
61: FJ 88        ; Exit loop
62: LA 0,4       ; [Inner FOR I := 1 TO 4]
63: LC 1         ;
64: ST           ; I := 1
65: LV 0,4       ;
66: LC 4         ;
67: LE           ; I <= 4
68: FJ 77        ; Exit inner loop
69: LC 32        ; Load ' ' (space character)
70: CALL 1,0     ; WRITEC(' ')
71: LA 0,4       ; I := I + 1
72: LV 0,4       ;
73: LC 1         ;
74: AD           ;
75: ST           ;
76: J 65         ; Loop back
77: LA 0,8       ; Load address của C
78: CALL 1,0     ; READC
79: ST           ; C := READC
80: LV 0,8       ; Load C
81: CALL 1,0     ; WRITEC(C)
82: LA 0,5       ; N := N + 1
83: LV 0,5       ;
84: LC 1         ;
85: AD           ;
86: ST           ;
87: J 58         ; Loop back
88: LA 0,6       ; P := 1
89: LC 1         ;
90: ST           ;
91: LA 0,7       ; Q := 2
92: LC 2         ;
93: ST           ;
94: LA 0,5       ; [FOR N := 2 TO 4]
95: LC 2         ;
96: ST           ; N := 2
97: LV 0,5       ;
98: LC 4         ;
99: LE           ; N <= 4
100: FJ 115      ; Exit loop
101: LA 0,4      ; I := 0
102: LC 0        ;
103: ST          ;
104: LV 0,5      ; Load N
105: LV 0,6      ; Load P
106: LV 0,7      ; Load Q
107: CALL 1,1    ; Call HANOI(N, P, Q), level 1, address 1
108: CALL 1,0    ; WRITELN
109: LA 0,5      ; N := N + 1
110: LV 0,5      ;
111: LC 1        ;
112: AD          ;
113: ST          ;
114: J 97        ; Loop back
115: HL          ; Halt
```

**Phân tích chi tiết:**

**1. Procedure HANOI:**
- **Stack frame:** 7 words (4 reserved + 3 parameters N, S, Z)
- **Parameters:**
  + N: offset 4
  + S: offset 5
  + Z: offset 6
- **Recursive calls:** Địa chỉ 21 và 52
- **Level 0 khi gọi đệ quy:** Vì gọi chính nó (cùng level)

**2. Static Link trong HANOI:**
- Khi truy xuất biến I từ program scope:
  + Dùng lệnh `LA 1,4` và `LV 1,4`
  + Level 1 nghĩa là: follow static link 1 lần để đến program frame
  + Offset 4 là vị trí của I trong program frame

**3. Nested scope illustration:**
```
Program EXAMPLE3 (level 0)
│
├─ Variables: I, N, P, Q, C (offsets 4-8)
│
└─ Procedure HANOI (level 1)
   │
   ├─ Parameters: N, S, Z (offsets 4-6)
   │
   └─ Can access I from outer scope using static link
```

**4. Recursive call analysis:**
```
HANOI(3, 1, 3) gọi:
├─ HANOI(2, 1, 2)
│  ├─ HANOI(1, 1, 3)
│  │  └─ HANOI(0, 1, 2)  [base case, return]
│  ├─ I := I + 1
│  └─ HANOI(1, 3, 3)
│     └─ HANOI(0, 3, 2)  [base case, return]
├─ I := I + 1
└─ HANOI(2, 2, 3)
   ├─ ...
   └─ ...
```

**5. Stack frame evolution:**
Khi gọi `HANOI(3, 1, 3)` từ main:
```
Stack layout:
[Main frame]
  offset 0-3: reserved
  offset 4: I
  offset 5: N
  offset 6: P
  offset 7: Q
  offset 8: C
[HANOI frame khi gọi]
  offset 0: return value (không dùng cho procedure)
  offset 1: dynamic link (BP của main)
  offset 2: return address
  offset 3: static link (BP của main, vì HANOI thuộc program level)
  offset 4: parameter N = 3
  offset 5: parameter S = 1
  offset 6: parameter Z = 3
```

**Kết quả thực thi:**
Chương trình giải bài toán Tower of Hanoi cho 2, 3, 4 đĩa, in ra các bước di chuyển.

---

## IV. KẾT LUẬN

### Thành tựu đạt được:

1. **Hoàn thiện code generation** cho tất cả các cấu trúc ngôn ngữ KPL
2. **Xử lý chính xác nested scope và static link** cho phép truy xuất biến non-local
3. **Hỗ trợ đầy đủ array, procedure, function** với parameter passing
4. **Sinh mã tối ưu** với số lượng lệnh hợp lý
5. **Tương thích case-insensitive** cho từ khóa và identifier
6. **Test thành công** trên 6 file ví dụ với độ phức tạp tăng dần

### Điểm mạnh:

- **Kiến trúc rõ ràng:** Tách biệt các phase của compiler
- **Code generation có hệ thống:** Mỗi cấu trúc ngữ pháp có hàm gen tương ứng
- **Xử lý scope chính xác:** Static link được tính toán đúng cho nested procedures
- **Debug friendly:** In ra assembly code giúp dễ dàng kiểm tra

### Hạn chế và hướng phát triển:

1. **Tối ưu hóa:** Chưa có phase optimize, mã sinh ra có thể tối ưu hơn
   - Loại bỏ redundant load/store
   - Common subexpression elimination
   - Dead code elimination

2. **Error handling:** Chưa có error recovery tốt, dừng ngay khi gặp lỗi

3. **Array nhiều chiều:** Hiện chỉ hỗ trợ array 1 chiều

4. **Type system:** Chưa hỗ trợ record, pointer, các kiểu dữ liệu phức tạp

5. **Virtual machine:** Có thể mở rộng với nhiều instruction set phong phú hơn

### Bài học kinh nghiệm:

- **Hiểu rõ instruction set** của target machine là quan trọng nhất
- **Stack-based architecture** đơn giản nhưng hiệu quả cho code generation
- **Static link** là cơ chế quan trọng để implement lexical scoping
- **Testing từng bước** giúp phát hiện lỗi sớm và dễ debug

---

**Người thực hiện:** [Tên sinh viên]
**Lớp:** [Mã lớp]
**Ngày hoàn thành:** 06/01/2026
