<?php
class dht11{
    public $link='';
    function __construct(){
        $connection = $this->connect();
        $this->storeInDB($_GET['sensor_value'], $_GET['phValue'], $_GET['temperatureValue'], $_GET['tdsValue'], $_GET['turbidityValue'], $connection);
    }

    function connect(){
        $con=mysqli_init();
        mysqli_ssl_set($con, NULL, NULL, "../certs/BaltimoreCyberTrustRoot.crt.pem", NULL, NULL);
        mysqli_real_connect($con, 'ponderdb.mysql.database.azure.com', 'db_admin@ponderdb', 'P0nd3rDB!', 'ponder', 3306);
        if(mysqli_connect_errno($con)){
            die('Failed to connect to MySQL: '.mysqli_connect_error());
        }
        else{
            printf("MySQL successfully");
        }
        return $con;

    }


    function storeInDB($sensor_value, $phValue, $temperatureValue, $tdsValue, $turbidityValue, $conn){
        date_default_timezone_set("Europe/London");
        $currentTime = date("Y-m-d h:i:s");
        $sql = "INSERT INTO temperature (sensorID, value, time) VALUES ('$sensor_value', '$temperatureValue', '$currentTime')";
        $sql2 = "INSERT INTO ph (sensorID, value, time) VALUES ('$sensor_value', '$phValue', '$currentTime')";
        $sql3 = "INSERT INTO tds (sensorID, value, time) VALUES ('$sensor_value', '$tdsValue', '$currentTime')";
        $sql4 = "INSERT INTO turbidity (sensorID, value, time) VALUES ('$sensor_value', '$turbidityValue', '$currentTime')";
        if(mysqli_query($conn, $sql)){
            echo "Temperatre data added successfully.\r\n";
        } else{
            echo "ERROR: Could not able to execute $sql for temperature data.\r\n" . mysqli_error($conn);
        }
        if(mysqli_query($conn, $sql2)){
            echo "ph data added successfully.\r\n";
        } else{
            echo "ERROR: Could not able to execute $sql for ph data.\r\n" . mysqli_error($conn);
        }
        if(mysqli_query($conn, $sql3)){
            echo "tds data added successfully.\r\n";
        } else{
            echo "ERROR: Could not able to execute $sql for tds data.\r\n" . mysqli_error($conn);
        }
        if(mysqli_query($conn, $sql4)){
            echo "turbidity data added successfully.\r\n";
        } else{
            echo "ERROR: Could not able to execute $sql for turbidity data.\r\n" . mysqli_error($conn);
        }
    }
 


}
$dht11=new dht11()

?>
