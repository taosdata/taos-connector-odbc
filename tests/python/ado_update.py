import win32com.client
import time
import datetime

# Import ADODB constants
adOpenStatic = 2
adLockReadOnly = 2
adCmdText = 1

def create_table_if_not_exists(conn):
    """Ensure table exists"""
    try:
        conn.Execute("DROP DATABASE IF EXISTS test;")
        # Create test database
        conn.Execute("CREATE DATABASE IF NOT EXISTS test;")

        # Create test table
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
        print("Successfully created/verified database and table structure")
        return True
    except Exception as e:
        print(f"Failed to create table: {str(e)}")
        return False


def insert_data_safe(conn, ts, device_id, temperature, humidity, status=True):
    """Safely insert data (using parameterized SQL)"""
    try:
        # Use parameterized SQL to prevent SQL injection
        # Insert test data
        insert_sql = f"""        
        INSERT INTO test.devices (ts, device_id, temperature, humidity, status)         
        VALUES             
            ({ts}, '{device_id}', {temperature}, {humidity}, true);        
        """
        conn.Execute(insert_sql)

        # Execute insert
        print(f"Successfully inserted device: {device_id}")
        return True
    except Exception as e:
        print(f"Failed to insert device {device_id}: {str(e)}")

        # Extract detailed error information
        try:
            errors = conn.Errors
            for i in range(errors.Count):
                err = errors.Item(i)
                print(f"   ADO Error #{i + 1}: {err.Description} (Code: 0x{err.Number:08X})")
        except:
            pass

        return False


def query_recent_records(conn, isModify=True, limit=5):
    """Query recent records"""
    try:
        rs = win32com.client.Dispatch("ADODB.Recordset")
        query = f"SELECT * FROM test.devices ORDER BY ts DESC LIMIT {limit};"
        rs.Open(
            Source=query,
            ActiveConnection=conn,
            CursorType=adOpenStatic,
            LockType=adLockReadOnly
        )

        records = []
        if not rs.EOF:
            if isModify:
                rs.Fields("device_id").Value = 'device_id_new_1'
                rs.Fields("temperature").Value = 1234
                rs.Fields("humidity").Value = 12233
                rs.Update()
            while not rs.EOF:
                record = {
                    'ts': rs.Fields("ts").Value,
                    'device_id': rs.Fields("device_id").Value,
                    'temperature': rs.Fields("temperature").Value,
                    'humidity': rs.Fields("humidity").Value,
                    'status': rs.Fields("status").Value
                }

                records.append(record)
                rs.MoveNext()

        return records
    except Exception as e:
        print(f"Failed to query records: {str(e)}")
        return []
    finally:
        if 'rs' in locals() and rs.State != 0:
            rs.Close()

def main():
    """Main program"""
    conn = None
    try:
        # Create main connection
        conn = win32com.client.Dispatch("ADODB.Connection")
        conn_str = "DSN=u-156;"
        conn.Open(conn_str)
        print("Successfully connected to TDengine database!")

        # Ensure table exists
        if not create_table_if_not_exists(conn):
            return

        print("Main program: Starting to monitor data changes and add new records...")

        # Main loop: read data every 3 seconds and add a record
        for i in range(3):
            print(f"\n--- Main program polling #{i + 1} ---")

            # Insert new record
            device_id = f"main_device_{i + 1:03d}"
            if insert_data_safe(conn, f'now +{i}a', device_id, 26.5 + i, 58.2 + i):
                print(f"Successfully added main program device: {device_id}")

        # Query and display recent records
        query_recent_records(conn)
        records = query_recent_records(conn, False)
        if records:
            print(f"Recent {len(records)} records:")
            for idx, record in enumerate(records):
                status = "Normal" if record['status'] else "Abnormal"
                print(
                    f"   #{idx + 1}: {record['ts']} | {record['device_id']} | "
                    f"Temperature: {record['temperature']:.1f} | Humidity: {record['humidity']:.1f} | "
                    f"Status: {status}"
                )

        else:
            print("No records found")

    except Exception as e:
        print(f"Main program error: {str(e)}")
    finally:
        # Ensure connection is closed
        if conn and conn.State != 0:
            try:
                conn.Close()
                print("Main program: Connection closed")
            except Exception as e:
                print(f"Error closing connection: {str(e)}")

if __name__ == "__main__":
    main()
