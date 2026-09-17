money = int(input("Nhap so tiền bạn có: "))
if (money < 0):
    print ("Số tiền không hợp lệ!")
print ("Đơn giá SP: ")
print ("Táo: 900 vnd")
print ("Cam: 800 vnd")
print ("Quýt: 500 vnd")  

print ("Sl sản phẩm mà bạn muốn mua là:")
sl_tao = int(input("SL táo: "))
sl_cam = int(input("SL cam: "))
sl_quyt = int(input("SL quýt: "))

tao = 900
cam = 800
quyt= 500  

tong = tao * sl_tao + cam * sl_cam + quyt * sl_quyt

print ("Số tiền bạn phải trả là: ", tong, "vnd")

if (money < tong):
    print ("Bạn không đủ tiền để thực hiện giao dịch!")
else:
    con_lai= money - tong
    print ("Số tiền của bạn còn lại là: ", con_lai, "vnd")

print ("Cảm ơn bạn đã mua hàng!")    
