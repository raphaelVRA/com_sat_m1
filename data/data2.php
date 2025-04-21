<?php
header('Content-Type: application/json');
header("Access-Control-Allow-Origin: *");

// Connexion à la base de données
$host = "localhost";
$user = "root";  // Change si tu as un autre utilisateur
$password = "root";  // Change si tu as mis un mot de passe
$dbname = "capteurs2";

$conn = new mysqli($host, $user, $password, $dbname);

// Vérifie la connexion
if ($conn->connect_error) {
    die(json_encode(["error" => "Échec de connexion à la base de données"]));
}

// Requête pour récupérer **toutes les mesures** (sans LIMIT)
$sql = "SELECT timestamp, temperature, pression, altitude, humidite FROM mesures ORDER BY timestamp ASC";
$result = $conn->query($sql);

$data = [];

if ($result->num_rows > 0) {
    while ($row = $result->fetch_assoc()) {
        $data[] = $row;
    }
}

// Retourne les données en JSON
echo json_encode($data);

$conn->close();
?>
