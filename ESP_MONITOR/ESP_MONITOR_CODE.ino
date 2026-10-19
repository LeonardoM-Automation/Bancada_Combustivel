//CÓDIGO DO ESP RESPONSAVEL ENVIAR O COMANDO DE ACIONAMENTO DO PROTÓTIPO, RECEBER OS DADOS MEDIDOS E SALVALOS EM UM ARQUIVO TXT PARA SEREM ANALISADOS

#include <WiFi.h>
#include <esp_now.h>
#define acionador 4
#define MAGIC_NUMBER 0x740
uint8_t esp_sensor[] = {0x08,0xF9,0xE0,0xE2,0x1C,0xF4};//endereço MAC
bool status_atual = false;
int acionador_antes = 1;
uint32_t ultimoAperto = 0;
volatile bool chegou = false;


//ESTRUTURA DE DADOS PARA COMUNICAÇÂO=============================================================================================================
#pragma pack(push,1)
struct CODE_ACIONAMENTO
{
  uint16_t magic;
  bool status;
};
struct DADOS
{
  uint16_t magic;
  float medida;
  uint32_t tempo;
};
#pragma pack(pop)
//================================================================================================================================================


DADOS msg;


//FUNÇÃO QUE RECEBE OS DADOS======================================================================================================================
void receive(const esp_now_recv_info *info, const uint8_t *data, int len)
{
  if(len!=sizeof(DADOS))return;
  
  memcpy(&msg,data,len);
  if(msg.magic!=MAGIC_NUMBER)return;
  chegou = true;
}
//=================================================================================================================================================




//CONFIGURAÇÕES====================================================================================================================================
void setup()
{
  
  WiFi.mode(WIFI_STA);
  esp_now_init();
  Serial.begin(115200);


  //INFORMAÇÔES DO PEER
  esp_now_peer_info_t sensorInfo = {};
  memcpy(sensorInfo.peer_addr, esp_sensor, 6);
  sensorInfo.channel = 0;
  sensorInfo.encrypt = false;
  esp_now_add_peer(&sensorInfo);

  
  esp_now_register_recv_cb(receive);
  pinMode(acionador,INPUT_PULLUP);
}
//=================================================================================================================================================





void loop() 
{
  if(digitalRead(acionador)==LOW && acionador_antes == 0 && millis() - ultimoAperto > 1000)
  {
    ultimoAperto = millis();
    status_atual=!status_atual;
    Serial.println("enviou");
    CODE_ACIONAMENTO code;
    code.status=status_atual;
    code.magic=MAGIC_NUMBER;
    esp_now_send(esp_sensor,(uint8_t*)&code,sizeof(code));
    acionador_antes = 1;
  }
  if(digitalRead(acionador) == HIGH)
  {
    acionador_antes = 0;
  }
  if(chegou)
  {
    Serial.print(msg.medida);
    Serial.print(",");
    Serial.println(msg.tempo);
    chegou=!chegou;
  }
}


