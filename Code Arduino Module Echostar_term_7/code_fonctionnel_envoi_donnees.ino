/**
 * @brief Code fusionné : Connexion satellite + Envoi des données BME280 via AT+SEND.
 *
 * - Se connecte au satellite (EM2050.ino intact).
 * - Lit les données du capteur BME280.
 * - Envoie les données avec `AT+SEND`.
 *
 * @author rvrain
 * @version 2.0.0
 */

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <MicroNMEA.h>

// Définition du capteur BME280
Adafruit_BME280 bme;
#define SEALEVELPRESSURE_HPA (1013.25)

// Variables de contrôle
#define SWITCH_REVERSING_CONTROL_DEFAULT_VALUE true
volatile int switch_reversing_control = SWITCH_REVERSING_CONTROL_DEFAULT_VALUE;

void do_switch_ctrl_update(void){
  if (digitalRead(ECHOSTAR_SWCTRL_PIN) == LOW) {
    digitalWrite(DPDT_CTRL_PIN, switch_reversing_control ? HIGH : LOW);
    digitalWrite(LED_BUILTIN, switch_reversing_control ? HIGH : LOW);
  } else {
    digitalWrite(DPDT_CTRL_PIN, switch_reversing_control ? LOW : HIGH);
    digitalWrite(LED_BUILTIN, switch_reversing_control ? LOW : HIGH);
  }
}

void setup(void)
{
  switch_reversing_control = SWITCH_REVERSING_CONTROL_DEFAULT_VALUE;

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

#if defined(ECHOSTAR_PWR_ENABLE_PIN)
  pinMode(ECHOSTAR_PWR_ENABLE_PIN, OUTPUT);
  digitalWrite(ECHOSTAR_PWR_ENABLE_PIN, HIGH);
#endif

#if defined(DPDT_PWR_ENABLE_PIN)
  pinMode(DPDT_PWR_ENABLE_PIN, OUTPUT);
  digitalWrite(DPDT_PWR_ENABLE_PIN, HIGH);
#endif

  pinMode(DPDT_CTRL_PIN, OUTPUT);
  digitalWrite(DPDT_CTRL_PIN, HIGH);

  pinMode(ECHOSTAR_SWCTRL_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ECHOSTAR_SWCTRL_PIN), swctrl_change_isr, CHANGE);

  USB_SERIAL.begin(115200);
  while (!USB_SERIAL);
  //USB_SERIAL.println("Starting...");

  do_switch_ctrl_update();
  //USB_SERIAL.print("RF Switch reverse control: ");
  //USB_SERIAL.println(switch_reversing_control ? "ENABLE" : "DISABLE");

  ECHOSTAR_SERIAL.begin(115200);
  delay(1000);

  digitalWrite(LED_BUILTIN, LOW);

  // Initialisation du BME280
  pinMode(SENSORS_PWR_ENABLE_PIN, OUTPUT);
  digitalWrite(SENSORS_PWR_ENABLE_PIN, HIGH);

  Wire.setSDA(SENSORS_I2C_SDA_PIN);
  Wire.setSCL(SENSORS_I2C_SCL_PIN);
  Wire.begin();

  if (!bme.begin(SENSORS_BME280_ADDRESS, &Wire)) {
    //USB_SERIAL.println("BME280 non détecté !");
    while (1) delay(10);
  }

  //USB_SERIAL.println("BME280 Initialisé !");
}

void loop(void)
{
  if (ECHOSTAR_SERIAL.available()) {
    USB_SERIAL.write(ECHOSTAR_SERIAL.read());
  }

  if (USB_SERIAL.available()) {
    char c = USB_SERIAL.read();
    ECHOSTAR_SERIAL.write(c);

    if (c == '*') {
      switch_reversing_control ^= true;
      do_switch_ctrl_update();

      //USB_SERIAL.print("RF Switch reverse control: ");
      //USB_SERIAL.println(switch_reversing_control ? "ENABLE" : "DISABLE");
    }
  }

  // Lecture et envoi des données BME280 toutes les 5 secondes
  static unsigned long lastSendTime = 0;
  if (millis() - lastSendTime >= 5000) {
    sendSensorData();
    lastSendTime = millis();
  }
}

void swctrl_change_isr(void)
{
  do_switch_ctrl_update();
}

// Fonction pour lire les données du capteur et les envoyer via AT+SEND
void sendSensorData() {
  // Construction du message AT+SEND directement à partir des lectures de bme
  char sendCommand[150];
  char tempStr[10], pressStr[10], altStr[10], humStr[10];
  dtostrf(bme.readTemperature(), 6, 2, tempStr);
  dtostrf(bme.readPressure() / 100.0F, 6, 2, pressStr); // hPa
  dtostrf(bme.readAltitude(SEALEVELPRESSURE_HPA), 6, 2, altStr);
  dtostrf(bme.readHumidity(), 6, 2, humStr);

  snprintf(sendCommand, sizeof(sendCommand),
         "AT+SEND=1,0,8,0,res:%s,%s,%s,%s\r\n",
         tempStr, pressStr, altStr, humStr);

  // Affichage dans le moniteur série
  //USB_SERIAL.println("Envoi des données BME280...");

  // Envoi de la commande via ECHOSTAR_SERIAL
  ECHOSTAR_SERIAL.print(sendCommand);
}
