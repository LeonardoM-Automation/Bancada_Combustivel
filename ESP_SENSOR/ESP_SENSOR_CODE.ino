//CÓDIGO DO ESP RESPONSAVEL POR REALIZAR O ACIONAMENTO, REALIZAR AS MEDIÇÕES E ENVIA-LÁS AO ESP CONTROLADOR

#include <WiFi.h>
#include <esp_now.h>
#include "HX711.h"

#define DT 19
#define SCK 18
#define starter 4
#define MAGIC_NUMBER 0x740
volatile bool status = false;
volatile bool mudeStatus = true;
uint32_t ultimoEnvio = 0;
uint8_t esp_controlador[] = {0x68,0x25,0xDD,0x48,0x8C,0x94};//endereço MAC

HX711 bancada;




//FILTRO===========================================================================================================================================
class Kelman 
{
  float q, r, x, p, k;
  public:
    Kelman(float q, float r, float initial_value) {
    this->q = q;
    this->r = r;
    this->x = initial_value;
    this->p = 1.0;
  }
  float update(float medida) {
    p += q;
    k = p / (p + r);
    x += k * (medida - x);
    p *= (1 - k);
    return x;
  }
};
Kelman filtro(0.01,1.0,0);
//=================================================================================================================================================





//ESTRUTURAS DE DADOS PARA COMUNICAÇÂO=============================================================================================================
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
//=================================================================================================================================================





//FUNÇÂO QUE RECEBE COMEANDO DE ACIONAMENTO========================================================================================================
void receive(const esp_now_recv_info *info, const uint8_t *data, int len)
{
  if(len!=sizeof(CODE_ACIONAMENTO))return;
  CODE_ACIONAMENTO code;
  memcpy(&code,data,len);
  if(code.magic!=MAGIC_NUMBER)return;
  status = code.status;
  mudeStatus = true;
}
//=================================================================================================================================================




//CONFIGURAÇÕES======================================================================================================================================
void setup()
{
  pinMode(starter, OUTPUT);
  digitalWrite(starter,LOW);
  WiFi.mode(WIFI_STA);
  esp_now_init();
  

//INFORMAÇÔES DO PEER
  esp_now_peer_info_t controladorInfo = {};
  memcpy(controladorInfo.peer_addr, esp_controlador, 6);
  controladorInfo.channel = 0;
  controladorInfo.encrypt = false;
  esp_now_add_peer(&controladorInfo);


//FUNÇÃO PARA RECEBER
  esp_now_register_recv_cb(receive);


//CONFIG BALANÇA
  bancada.begin(DT, SCK);
  bancada.set_scale(2280.0);
  bancada.tare();
}
//=================================================================================================================================================






void loop()
{

  if(millis() - ultimoEnvio > 12.5)//1000/80hz
  {
    ultimoEnvio = millis();

    //ENVIANDOO DADOS=========================================================================================================================
    if(bancada.is_ready())
    {
      DADOS msg;
      msg.magic = MAGIC_NUMBER;
      msg.medida = filtro.update(bancada.get_units(2));
      msg.tempo = millis();
      esp_now_send(esp_controlador,(uint8_t*)&msg,sizeof(msg));
    }
  }
  //LOGICA DE ACIONAMENTO DO INGNITOR=========================================================================================================================
  if(mudeStatus)
  {
    digitalWrite(starter,status? HIGH : LOW);
    mudeStatus = false;
  }
    
}
