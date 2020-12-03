<?php
class dht11{
    public $link='';
    function __construct($data1, $data2){
        $this->connect();
        $this->storeInDB($data1, $data2);
    }

    function connect(){
        $con=mysqli_init();
        mysqli_ssl_set($con, NULL, NULL, "/certs/DigiCertGlobalRootG2.crt.pem", NULL, NULL);
        mysqli_real_connect($con, 'ponderdb.mysql.database.azure.com', 'db_admin@ponderdb', 'P0nd3rDB!', 'test', 3306);
        if(mysqli_connect_errno($con)){
            die('Failed to connect to MySQL: '.mysqli_connect_error());
        }

 }

    function storeInDB($data1, $data2){
        $query = "insert into test set data2='".$data2."', data1='".$data1."'";
        $result = mysqli_query($this->link,$query) or die('Errant query:  '.$query);
        echo "success";
        echo $result;
    }

}
if($_GET['data1'] != '' and  $_GET['data2'] != ''){
    $dht11=new dht11($_GET['data1'],$_GET['data2']);
}

?>
