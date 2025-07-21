import win32com.client
import pythoncom
import os

# Define ADO constants
adUseClient = 3
adCursorType = 2
adLockType = 2

def connect_tdengine():
    print("Attempting to connect to TDengine database...")

    try:
        # Create ADO objects
        conn = win32com.client.Dispatch("ADODB.Connection")
        rs = win32com.client.Dispatch("ADODB.Recordset")

        # Use DSN connection string
        conn_str = "DSN=u-156;"
        # Open connection
        conn.Open(conn_str)
        print(f"--Successfully connected to TDengine database!")


        # Create test database
        conn.Execute("DROP DATABASE IF EXISTS test;")
        conn.Execute("CREATE DATABASE IF NOT EXISTS test;")
        conn.Execute("USE test;")

        # Create test table
        create_table_sql = """
        CREATE TABLE IF NOT EXISTS devices (
            ts TIMESTAMP,
            device_id NCHAR(20),
            temperature FLOAT,
            humidity FLOAT,
            status BOOL
        );
        """
        conn.Execute(create_table_sql)

        # Insert test data
        insert_sql = """
        INSERT INTO devices (ts, device_id, temperature, humidity, status)
        VALUES
            (now, 'device_001', 25.3, 45.2, true),
            (now-10s, 'device_002', 26.8, 42.7, false),
            (now-20s, 'device_003', 24.1, 47.9, true),
            (now-30s, 'device_003', 25.1, 47.9, true),
            (now-40s, 'device_003', 26.1, 47.9, true);
        """
        conn.Execute(insert_sql)
        print("Successfully inserted test data")

        # Query data
        query = "SELECT * FROM test.devices;"
        rs.Open(query, conn)

        # Display results
        print("\nQuery results:")
        print(f"{'Timestamp':<25} {'Device ID':<15} {'Temp':<8} {'Humidity':<8} {'Status'}")
        print("-" * 65)

        while not rs.EOF:
            ts = rs.Fields("ts").Value
            device_id = rs.Fields("device_id").Value
            temperature = rs.Fields("temperature").Value
            humidity = rs.Fields("humidity").Value
            status = "Normal" if rs.Fields("status").Value else "Abnormal"

            print(f"{ts:<25} {device_id:<15} {temperature:<8.1f} {humidity:<8.1f} {status}")
            rs.MoveNext()

        # Clean up resources
        rs.Close()
        conn.Close()
        print("\nConnection closed")

    except Exception as e:
        print(f"Error occurred: {str(e)}")
        # Extract more error information
        if 'conn' in locals():
            errors = conn.Errors
            for i in range(errors.Count):
                err = errors.Item(i)
                print(f" ADO Error #{i + 1}:")
                print(f" Description: {err.Description}")
                print(f" Code: {err.Number} (0x{err.Number:08X})")
                print(f" Source: {err.Source}")

        # Ensure connection is closed
        if 'rs' in locals() and rs.State != 0:
            rs.Close()
        if 'conn' in locals() and conn.State != 0:
            conn.Close()

def query_tdengine():
    try:
        # Create ADO objects
        conn = win32com.client.Dispatch("ADODB.Connection")
        rs = win32com.client.Dispatch("ADODB.Recordset")

        # Use DSN connection string
        conn = win32com.client.Dispatch("ADODB.Connection")
        # Use DSN connection string
        conn_str = "DSN=TAOS_ODBC_WS_DSN;"
        # Open connection
        conn.Open(conn_str)
        print(f"--Successfully connected to TDengine database!")
        rs = win32com.client.Dispatch("ADODB.Recordset")
        rs.CursorLocation = adUseClient  # Must set to client cursor
        query = "SELECT * FROM test.devices;"
        rs.Open(query, conn, adCursorType, adLockType)

        print(f"Total records: {rs.RecordCount}")
        rs.Filter = "device_id = 'device_003'"
        rs.Sort = "temperature DESC"

        print("\nFilter: device_id = 'device_003' Sort: temperature DESC")
        print(f"Filtered record count: {rs.RecordCount}")
        while not rs.EOF:
            ts = rs.Fields("ts").Value
            device_id = rs.Fields("device_id").Value
            temperature = rs.Fields("temperature").Value
            humidity = rs.Fields("humidity").Value
            status = "Normal" if rs.Fields("status").Value else "Abnormal"

            print(f"{ts:<25} {device_id:<15} {temperature:<8.1f} {humidity:<8.1f} {status}")
            rs.MoveNext()

    except Exception as e:
        print(f"Error occurred: {str(e)}")
    finally:
        # Ensure connection is closed
        if 'rs' in locals() and rs.State != 0:
            rs.Close()
        if 'conn' in locals() and conn.State != 0:
            conn.Close()

if __name__ == "__main__":
    # Initialize COM environment
    pythoncom.CoInitialize()
    connect_tdengine()
    query_tdengine()
    pythoncom.CoUninitialize()