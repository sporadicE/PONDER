<?php
class dht11{
    public $link='';
    function __construct(){
        $connection = $this->connect();
        $this->storeInDB($_GET['sensor_value'], $_GET['sensor_status'], $connection);
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


    function storeInDB($sensor_value, $sensor_status, $conn){
        date_default_timezone_set("Europe/London");
        $currentTime = date("Y-m-d h:i:s");
        $sql = "INSERT INTO sensor (id, status, timeon) VALUES ('$sensor_value', '$sensor_status', '$currentTime')";
        if(mysqli_query($conn, $sql)){
            echo "Sensor added successfully.\r\n";
        } else{
            echo "ERROR: Could not able to execute $sql for sensor .\r\n" . mysqli_error($conn);
        }
    }
 


}
$dht11=new dht11()

?>
