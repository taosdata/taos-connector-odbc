import win32com.client
import pythoncom
import os

# 定义 ADO 常量
adUseClient = 3
adOpenStatic = 3
adLockReadOnly = 1
adCmdText = 1
adFilterNone = 0
adFilterPendingRecords = 1
adFilterAffectedRecords = 2
adFilterFetchedRecords = 3
adFilterConflictingRecords = 5

def connect_tdengine():
    print("尝试连接 TDengine 数据库...")

    try:
        # 创建 ADO 对象
        conn = win32com.client.Dispatch("ADODB.Connection")
        rs = win32com.client.Dispatch("ADODB.Recordset")

        # 使用 DSN 连接字符串
        conn_str = "DSN=u-156;"
        # 打开连接
        conn.Open(conn_str)
        print(f"--成功连接到 TDengine 数据库!")


        # # 创建测试数据库
        # conn.Execute("CREATE DATABASE IF NOT EXISTS test;")
        # conn.Execute("USE test;")
        #
        # # 创建测试表
        # create_table_sql = """
        # CREATE TABLE IF NOT EXISTS devices (
        #     ts TIMESTAMP,
        #     device_id NCHAR(20),
        #     temperature FLOAT,
        #     humidity FLOAT,
        #     status BOOL
        # );
        # """
        # conn.Execute(create_table_sql)
        #
        # # 插入测试数据
        # insert_sql = """
        # INSERT INTO devices (ts, device_id, temperature, humidity, status)
        # VALUES
        #     (now, 'device_001', 25.3, 45.2, true),
        #     (now-10s, 'device_002', 26.8, 42.7, false),
        #     (now-20s, 'device_003', 24.1, 47.9, true);
        # """
        # conn.Execute(insert_sql)
        # print("成功插入测试数据")

        # 查询数据
        query = "SELECT * FROM test.devices;"
        rs.Open(query, conn)

        # 显示结果
        print("\n查询结果:")
        print(f"{'时间戳':<25} {'设备ID':<15} {'温度':<8} {'湿度':<8} {'状态'}")
        print("-" * 65)

        while not rs.EOF:
            rs.Edit()
            print("open edit")
            ts = rs.Fields("ts").Value
            device_id = rs.Fields("device_id").Value
            temperature = rs.Fields("temperature").Value
            humidity = rs.Fields("humidity").Value
            status = "正常" if rs.Fields("status").Value else "异常"

            print(f"{ts:<25} {device_id:<15} {temperature:<8.1f} {humidity:<8.1f} {status}")
            rs.MoveNext()
            break

        # 清理资源
        rs.Close()
        conn.Close()
        print("\n连接已关闭")

    except Exception as e:
        print(f"发生错误: {str(e)}")
        # 提取更多错误信息
        if 'conn' in locals():
            errors = conn.Errors
            for i in range(errors.Count):
                err = errors.Item(i)
                print(f"  ADO 错误 #{i + 1}:")
                print(f"    描述: {err.Description}")
                print(f"    代码: {err.Number} (0x{err.Number:08X})")
                print(f"    源: {err.Source}")

        # 确保关闭连接
        if 'rs' in locals() and rs.State != 0:
            rs.Close()
        if 'conn' in locals() and conn.State != 0:
            conn.Close()

def query_tdengine():
    try:
        # 创建 ADO 对象
        conn = win32com.client.Dispatch("ADODB.Connection")
        # 使用 DSN 连接字符串
        conn_str = "DSN=u-156;"
        # 打开连接
        conn.Open(conn_str)
        print(f"--成功连接到 TDengine 数据库!")
        rs = win32com.client.Dispatch("ADODB.Recordset")
        rs.CursorLocation = adUseClient  # 必须设置为客户端游标
        query = "SELECT * FROM test.devices;"
        rs.Open(query, conn, adOpenStatic, adLockReadOnly, adCmdText)

        print(f"总记录数: {rs.RecordCount}")
        rs.Filter = "device_id = 'main_device_003'"
        rs.Sort = "temperature DESC"

        print("\n筛选: device_id = 'main_device_003'")
        print(f"筛选后记录数: {rs.RecordCount}")
        while not rs.EOF:
            # rs.Fields("device_id").Value = "xxxx-1"
            # rs.Update()
            # print(f"主程序: 成功添加设备 {device_id}")
            # break
            ts = rs.Fields("ts").Value
            device_id = rs.Fields("device_id").Value
            temperature = rs.Fields("temperature").Value
            humidity = rs.Fields("humidity").Value
            status = "正常" if rs.Fields("status").Value else "异常"

            print(f"{ts:<25} {device_id:<15} {temperature:<8.1f} {humidity:<8.1f} {status}")
            rs.MoveNext()


    except Exception as e:
        print(f"发生错误: {str(e)}")
    finally:
        # 确保关闭连接
        if 'rs' in locals() and rs.State != 0:
            rs.Close()
        if 'conn' in locals() and conn.State != 0:
            conn.Close()

if __name__ == "__main__":
    # 初始化 COM 环境
    pythoncom.CoInitialize()
    #connect_tdengine()
    query_tdengine()
    pythoncom.CoUninitialize()
