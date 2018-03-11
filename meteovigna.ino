// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_DHT_Particle.h>

// This #include statement was automatically added by the Particle IDE.
//#include <PietteTech_DHT/PietteTech_DHT.h>


#define DHTTYPE  DHT22       // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPIN   D3          // Digital pin for communications with AM2301
#define LIGHTPIN A0         // Analog pin for photoresistence
#define WINDPIN D2           // Digital pin for wind measurement 
#define DROPPIN1 A1         // Analog pin for drop sensor #1
#define DROPPIN2 A2         // Analog pin for drop sensor #2
#define MOISTPIN A4         // Analog pin for moisture sensor
#define TRTPIN D0          // digital pin to activate the drop and wind sensors

int sleepTime = 900;   // si risveglia ogni 15 minuti, 7200 == ogni 2 ore

// Lib instantiate
DHT DHT(DHTPIN, DHTTYPE);
int n;      // counter
long previousMillis = 0;
long interval = 10000;         //millisecondi tra ogni invio

// variabili per l'anemometro
float windspeed = 0;
const float Pi = 3.141593; // Pigreco
const float raggio = 0.06; // raggio dell'anemometro in metri
int Statoreed = 0; // variabile per la lettura del contatto 
int Statoreed_old = 0; // variabile per evitare doppio conteggio
int Conteggio = 0;// variabile che contiene il conteggio delle pulsazioni
unsigned long int TempoStart = 0; // memorizza i  millisecondi dalla prima pulsazione conteggiata
unsigned long int Tempo = 0; // per conteggiare il tempo trascorso dalla prma pulsazione
unsigned long int TempoMax = 2000;// numero di millisecondi (2 seondi) per azzerare il conteggio e calcolare la Velocità
                  // così non si conteggiano brevi folate di vento

// lowest and highest sensor readings:
const int sensorMin = 0;     // sensor minimum
const int sensorMax = 4096;  // sensor maximum

// variabili per fotoresistore
float frValue = 0; 

// variabili per lettura stato batteria                  
FuelGauge fuel; //
float volt = 0;
float soc = 0;

float umi = 0;
float temp = 0;
float dew = 0;
int rain = 4; // 4 = error/startup, 3 = no rain, 2 = light rain, 1 normal rain, 0 flood
int moisture = 0;
    

// variabile per pubblicare
char *publishStr;

void setup()
{
    Serial.begin(9600);
    pinMode(WINDPIN, INPUT);
    pinMode(TRTPIN, OUTPUT);
    pinMode(LIGHTPIN, INPUT);
    
    while (!Serial.available() && millis() < 30000) {
        Serial.println("Press any key to start.");
		Particle.process();
        delay (1000);
    }
    DHT.begin();
    // Particle.publish("V", DHTLIB_VERSION, 60, PRIVATE);
}


void loop()
{
    digitalWrite(TRTPIN, HIGH); // accende i sensori tramite transistor
    delay(1000);
    getBattery();
    delay(500);
    getLight();
    delay(500);
    getWind2();
    delay(500);
    getDrop();
    delay(500);
    getMoisture();
    delay(500);
    getDHT();
    delay(500);
    digitalWrite(TRTPIN, LOW); // spegne i sensori tramite transistor
    // pubblica i valori separati da virgola
    // volt, soc, frValue, umi, temp, pioggia, windspeed, umiterreno
    Particle.publish("V", String::format("%.1f %.0f %.0f %.0f %.1f %d %.0f %d",volt, soc, frValue, umi, temp, rain, windspeed, moisture) , 60, PRIVATE);

    delay(10000);
    System.sleep(SLEEP_MODE_DEEP, sleepTime); // pubblica i dati ogni 2 ore - inserire controllo su stato carica batteria, sleep 12 ore se soc < 20%
}

void getBattery() {
    volt = fuel.getVCell();
    soc = fuel.getSoC(); 
}

void getLight() {
    frValue = analogRead(LIGHTPIN);
}

void getDrop() {

    delay(500);
    int sensorReading1 = analogRead(DROPPIN1);
    delay(200);
    int sensorReading2 = analogRead(DROPPIN2);
    int sensorReading = (sensorReading1 + sensorReading2) / 2;
    // rain = map(sensorReading, sensorMin, sensorMax, 0, 3);
    rain = sensorReading;

}

void getMoisture() {
    moisture = analogRead(MOISTPIN);
}

void getWind() {

  bool statusExit = false;

  while (not statusExit) {    
    // inserire loop per eseguire più misurazioni e fare la media?
    Statoreed = digitalRead(WINDPIN); // legge il contatto reed
    if (Statoreed != Statoreed_old) // si verifica se è cambiato
     {
       Statoreed_old = Statoreed;   // se SI si aggiorna lo stato
       if (Statoreed == HIGH)  // si controlla SE è alto ( passaggio magnete)
         {
           if (Conteggio == 0){ TempoStart =  millis();} // se E' il primo passaggio
                                                 // si memorizza il tempo di partenza
           Conteggio = Conteggio + 1; // si aggiorna il contatore     
   
           Tempo = ( millis() - TempoStart); // si conteggia il tempo trascorso dallo start conteggio
           if (Tempo >=  TempoMax)   // se il tempo trascorso è maggiore o uguale al tempo impostato si eseguono i calcoli e la stampa della velocità
             {
               float deltaTempo = ( Tempo/1000.0); // si trasforma in secondi
               float Metris = (Conteggio*Pi*raggio)/deltaTempo;      // si calcola la velocità in metri/s
               windspeed = Metris;
               // float Kmora = (3.6*Conteggio*Pi*raggio)/deltaTempo; //formula per il calcolo della velocitàin Km/h
               // float Node = (Metris/0.514444); // si porta in nodi
        
               Conteggio = 0; // azzeriamo il conteggio per nuova lettura
               statusExit = true;        
             }
         }
     } else {
               statusExit = true;
    }
  }
}    


void getWind2() {

    TempoStart =  millis(); // E' il primo passaggio si memorizza il tempo di partenza

           Tempo = ( millis() - TempoStart); // si conteggia il tempo trascorso dallo start conteggio
           while (Tempo <  TempoMax)   // se il tempo trascorso è maggiore o uguale al tempo impostato si eseguono i calcoli e la stampa della velocità
             {
                Statoreed = digitalRead(WINDPIN); // legge il contatto reed
                if (Statoreed != Statoreed_old) // si verifica se è cambiato
                {
                    Statoreed_old = Statoreed;   // se SI si aggiorna lo stato
                    if (Statoreed == HIGH)  // si controlla SE è alto ( passaggio magnete)
                        {
                            Conteggio = Conteggio + 1; // si aggiorna il contatore
                        }
                }
                Tempo = ( millis() - TempoStart);
             }
            float deltaTempo = ( Tempo/1000.0); // si trasforma in secondi
            float Metris = (Conteggio*Pi*raggio)/deltaTempo;      // si calcola la velocità in metri/s
            windspeed = Metris;
}

   
void  getDHT() {
    
//    int result = DHT.acquireAndWait(1000);
/*
    switch (result) {
        case DHTLIB_OK:
            Particle.publish("Message","OK", 60, PRIVATE);
            break;
        case DHTLIB_ERROR_CHECKSUM:
            Particle.publish("Message","DHT - Checksum error", 60, PRIVATE);
            break;
        case DHTLIB_ERROR_ISR_TIMEOUT:
            Particle.publish("Message","DHT - ISR time out error", 60, PRIVATE);
            break;
        case DHTLIB_ERROR_RESPONSE_TIMEOUT:
            Particle.publish("Message","DHT - Response time out error", 60, PRIVATE);
            break;
        case DHTLIB_ERROR_DATA_TIMEOUT:
            Particle.publish("Message","DHT - Data time out error", 60, PRIVATE);
            break;
        case DHTLIB_ERROR_ACQUIRING:
            Particle.publish("Message","DHT - Acquiring", 60, PRIVATE);
            break;
        case DHTLIB_ERROR_DELTA:
            Particle.publish("Message","DHT - Delta time too small", 60, PRIVATE);
            break;
        case DHTLIB_ERROR_NOTSTARTED:
            Particle.publish("Message","DHT - Not started", 60, PRIVATE);
            break;
        default:
            Particle.publish("Message","DHT - Unknown error", 60, PRIVATE);
            break;
    }
*/
    umi = DHT.getHumidity();
    temp = DHT.getTempCelcius();
    // dew = DHT.getDewPoint();

}

/*
    n++;
    }
*/

