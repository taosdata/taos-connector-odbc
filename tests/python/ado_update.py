import win32com.client
import time
import threading
from datetime import datetime

# 引用 ADODB 常量
adOpenDynamic = 2
adLockOptimistic = 2
adCmdTable = 2

def main():
    # 创建主连接
    conn = win32com.client.Dispatch("ADODB.Connection")
    conn_str = "DSN=u-156;"
    conn.Open(conn_str)
    print("成功连接到 TDengine 数据库!")

    # 创建记录集
    rs = win32com.client.Dispatch("ADODB.Recordset")

    try:
        # 使用表名方式打开记录集
        rs.Open(
            Source="test.devices",
            ActiveConnection=conn,
            CursorType=adOpenDynamic,
            LockType=adLockOptimistic,
            Options=adCmdTable
        )


        print("主程序: 开始监控数据变化并添加新记录...")

        # 主循环：每3秒读取一次数据并添加一条记录
        for i in range(5):
            print(f"\n--- 主程序轮询 #{i + 1} ---")

            # 添加新记录

            # 移动到记录集末尾以便添加新记录
            if not rs.EOF:
                rs.MoveLast()

            # # 添加新记录
            rs.AddNew()

            # 设置字段值
            device_id = f"main_device_{i + 1:03d}"
            rs.Fields("ts").Value = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")
            rs.Fields("device_id").Value = device_id
            rs.Fields("temperature").Value = 26.5 + i
            rs.Fields("humidity").Value = 58.2 + i
            rs.Fields("status").Value = True

            # 提交更改
            rs.Update()
            print(f"主程序: 成功添加设备 {device_id}")



            # 显示当前记录数
            try:
                if not rs.EOF:
                    rs.MoveFirst()
                    record_count = 0

                    # 只显示最近5条记录
                    print("\n最近5条记录:")
                    while not rs.EOF and record_count < 5:
                        ts = rs.Fields("ts").Value
                        device_id = rs.Fields("device_id").Value
                        temperature = rs.Fields("temperature").Value
                        humidity = rs.Fields("humidity").Value
                        status = "正常" if rs.Fields("status").Value else "异常"

                        print(f"  {ts} | {device_id} | 温度: {temperature:.1f} | 湿度: {humidity:.1f} | 状态: {status}")

                        rs.MoveNext()
                        record_count += 1
            except Exception as e:
                print(f"显示记录时出错: {str(e)}")

            time.sleep(3)
    except Exception as e:
        print(f"主程序错误: {str(e)}")
    finally:
        # 清理资源
        if rs.State != 0:
            rs.Close()
        if conn.State != 0:
            conn.Close()
        print("主程序: 已关闭连接")


if __name__ == "__main__":
    user_input = input("请输入内容：")  # 程序会在此处暂停，直到用户按下回车键
    print(f"你输入的是：{user_input}")
    main()
