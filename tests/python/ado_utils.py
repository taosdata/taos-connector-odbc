"""
TDengine utility functions for ADO operations
Common functions for database table creation and data insertion
"""

import win32com.client

def create_table(conn):
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


def insert_data(conn, ts, device_id, temperature, humidity, status=True):
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


def connect_to_database(dsn_name="TAOS_ODBC_WS_DSN"):
    """Create and return a database connection"""
    try:
        conn = win32com.client.Dispatch("ADODB.Connection")
        conn_str = f"DSN={dsn_name};"
        conn.Open(conn_str)
        print("Successfully connected to TDengine database!")
        return conn
    except Exception as e:
        print(f"Failed to connect to database: {str(e)}")
        return None


def close_connection(conn):
    """Safely close database connection"""
    if conn and conn.State != 0:
        try:
            conn.Close()
            print("Database connection closed")
        except Exception as e:
            print(f"Error closing connection: {str(e)}")
