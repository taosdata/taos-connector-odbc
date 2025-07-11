import win32com.client
import time
import datetime
# import threading

# 引用 ADODB 常量
adOpenStatic = 2
adLockReadOnly = 2
adCmdText = 1


def create_table_if_not_exists(conn):
    """确保表存在"""
    try:
        # 创建测试数据库
        conn.Execute("CREATE DATABASE IF NOT EXISTS test;")

        # 创建测试表
        create_table_sql = """
            CREATE TABLE IF NOT EXISTS test.devices (
                  ts TIMESTAMP,
                  device_id NCHAR(20),
                  temperature FLOAT,
                  humidity FLOAT,
                  status BOOL
            );
            """
        conn.Execute(create_table_sql)
        print("成功创建/验证数据库和表结构")
        return True
    except Exception as e:
        print(f"创建表失败: {str(e)}")
        return False


def insert_data_safe(conn, ts, device_id, temperature, humidity, status=True):
    """安全地插入数据（使用参数化 SQL）"""
    try:
        # 使用参数化 SQL 防止 SQL 注入

        # 创建命令对象
        # cmd = win32com.client.Dispatch("ADODB.Command")
        # cmd.ActiveConnection = conn

        # 插入测试数据
        insert_sql = f"""        
        INSERT INTO test.devices (ts, device_id, temperature, humidity, status)         
        VALUES             
            ({ts}, '{device_id}', {temperature}, {humidity}, true);        
        """
        conn.Execute(insert_sql)

        # 执行插入
        print(f"成功插入设备: {device_id}")
        return True
    except Exception as e:
        print(f"插入设备 {device_id} 失败: {str(e)}")

        # 提取详细错误信息
        try:
            errors = conn.Errors
            for i in range(errors.Count):
                err = errors.Item(i)
                print(f"   ADO 错误 #{i + 1}: {err.Description} (代码: 0x{err.Number:08X})")
        except:
            pass

        return False


def modify_data(rs, tbname):
    """在后台线程中模拟其他用户插入数据"""
    try:
        conn = win32com.client.Dispatch("ADODB.Connection")
        conn_str = "DSN=u-156;"
        conn.Open(conn_str)

        if not rs.EOF:
            ts = rs.Fields("ts").Value
            device_id = rs.Fields("device_id").Value
            temperature = rs.Fields("temperature").Value
            humidity = rs.Fields("humidity").Value
            status = rs.Fields("status").Value

            temperature = 25.0 + 100
            humidity = 40.0 + 100

            # 使用安全的插入函数
            insert_data_safe(conn, f"'{ts}'", device_id, temperature, humidity)

    except Exception as e:
        print(f"后台线程错误: {str(e)}")
    finally: 
        if 'conn' in locals() and conn.State != 0:
            conn.Close()


def query_recent_records(conn, limit=5):
    """查询最近的记录"""
    try:
        rs = win32com.client.Dispatch("ADODB.Recordset")
        query = f"SELECT * FROM test.devices ORDER BY ts DESC LIMIT {limit};"

        rs.Open(
            Source=query,
            ActiveConnection=conn,
            CursorType=adOpenStatic,
            LockType=adLockReadOnly,
            Options=adCmdText
        )

        records = []
        if not rs.EOF:
            # rs.MoveFirst()
            while not rs.EOF:
                #rs.AddNew()
                # current_ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")
                # rs.Fields("ts").Value = current_ts
                rs.Fields("device_id").Value = 'device_id_new_1'
                rs.Fields("temperature").Value = 1234
                rs.Fields("humidity").Value = 12233
                rs.Update()
                # rs.Fields("status").Value = True
                # record = {
                #     'ts': rs.Fields("ts").Value,
                #     'device_id': rs.Fields("device_id").Value,
                #     'temperature': rs.Fields("temperature").Value,
                #     'humidity': rs.Fields("humidity").Value,
                #     'status': rs.Fields("status").Value
                # }
                # modify_data(rs, 'test.devices')
                # records.append(record)
                # rs.MoveNext()

        return records
    except Exception as e:
        print(f"查询记录失败: {str(e)}")
        return []
    finally:
        if 'rs' in locals() and rs.State != 0:
            rs.Close()


def main():
    """主程序"""
    conn = None
    try:
        # 创建主连接
        conn = win32com.client.Dispatch("ADODB.Connection")
        conn_str = "DSN=u-156;"
        conn.Open(conn_str)
        print("成功连接到 TDengine 数据库!")

        # 确保表存在
        # if not create_table_if_not_exists(conn):
        #     return

        print("主程序: 开始监控数据变化并添加新记录...")

        # 主循环：每3秒读取一次数据并添加一条记录
        for i in range(1):
            print(f"\n--- 主程序轮询 #{i + 1} ---")

            # 插入新记录
            # device_id = f"main_device_{i + 1:03d}"
            # if insert_data_safe(conn, f'now +{i}a', device_id, 26.5 + i, 58.2 + i):
            #     print(f"成功添加主程序设备: {device_id}")

            # 查询并显示最近记录
            records = query_recent_records(conn)
            if records:
                print(f"最近 {len(records)} 条记录:")
                for idx, record in enumerate(records):
                    status = "正常" if record['status'] else "异常"
                    print(
                        f"   #{idx + 1}: {record['ts']} | {record['device_id']} | "
                        f"温度: {record['temperature']:.1f} | 湿度: {record['humidity']:.1f} | "
                        f"状态: {status}"
                    )

            else:
               print("未查询到任何记录")

            time.sleep(3)

    except Exception as e:
        print(f"主程序错误: {str(e)}")
    finally:
        # 确保关闭连接
        if conn and conn.State != 0:
            try:
                conn.Close()
                print("主程序: 已关闭连接")
            except Exception as e:
                print(f"关闭连接时出错: {str(e)}")


if __name__ == "__main__":
    main()