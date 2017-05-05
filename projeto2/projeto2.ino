/* EA076 - Projeto 2: Datalogger
 * Henrique e Daniel - Turma D
 *
 * Codigo parcialmente baseado no codigo de Kernel de tempo real e comunicacao UART de Tiago Tavares
 * 
 */

//Inclusao de bibliotecas
#include <stdio.h>
#include <Wire.h>
#include <TimerOne.h>

//Definicoes
#define LM35 A0
#define C1 2
#define C2 3
#define C3 4
#define L1 5
#define L2 6
#define L3 7
#define L4 8
#define LED 10
#define TAM_MAX_BUFFER 15

//Vetores de linhas e colunas
int linha[4] = {L1,L2,L3,L4};
int coluna[3] = {C1,C2,C3};

//Inicializacao da matriz do teclado
char teclado[4][3] = {{'1','2','3'},{'4','5','6'},{'7','8','9'},{'*','0','#'}};

//Endereco da EEPROM
const byte END_EEPROM = 0x50;

//buffer_serial de dados
typedef struct {
  char data[TAM_MAX_BUFFER];
  unsigned int tam_buffer;
} buffer;

buffer buffer_serial;
buffer buffer_teclado;

//Variaveis globais para controle de processos da interrupcao
volatile int flag_leu_string = 0;
volatile int flag_deboucing = 0;
volatile int flag_leu_teclado = 0;
volatile int tempo_deboucing;

//Rotina para comparacao de strings. Retorna 1 se sao iguais e 0 caso contrario.
int compara_string(char *string1, char *string2, int tam) {
  int i;
  for (i=0; i<tam; i++) {
    if (string1[i] != string2[i]) return 0;
    if (string1[i] == '\0') return 1;
  }
  return 1;
}

//Limpa o buffer serial
void limpa_Sbuffer() {
  buffer_serial.tam_buffer = 0;
}

//Limpa o buffer teclado
void limpa_Tbuffer() {
  buffer_teclado.tam_buffer = 0;
}

//Adiciona caractere ao buffer serial
int insere_Sbuffer(char c_in) {
  if (buffer_serial.tam_buffer < TAM_MAX_BUFFER) {
    buffer_serial.data[buffer_serial.tam_buffer++] = toupper(c_in);
    return 1;
  }
  return 0;
}

//Adiciona caractere ao buffer teclado
int insere_Tbuffer(char c_in) {
  Serial.println(c_in); //apagar depois <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  if (c_in == '*')
    flag_leu_teclado = 1;
  if (buffer_teclado.tam_buffer < TAM_MAX_BUFFER) {
    buffer_teclado.data[buffer_teclado.tam_buffer++] = c_in;
    return 1;
  }
  return 0;
}

//Leitura da temperatura
uint8_t le_temperatura(){
   return (float(analogRead(A0))*5/(1023))/0.01; //Converte o valor da leitura para o valor de temperatura em Celsius
}

//Escrita de um byte da memoria
void escreve_byte(byte end_eeprom, unsigned end_mem, byte dado){
  byte aux = end_eeprom | ((end_mem >> 8) & 0x07);
  Wire.beginTransmission(aux);
  Wire.write(int(end_mem));
  Wire.write(dado);
  Wire.endTransmission();
  delay(100);
}

//Leitura de um byte da memoria
int le_byte(byte end_eeprom, unsigned end_mem){
  byte dado = -1;
  
  byte aux = end_eeprom | ((end_mem >> 8) & 0x07);
  Wire.beginTransmission(aux);
  Wire.write(int(end_mem));
  Wire.endTransmission();
  Wire.requestFrom(int(aux),1);
  if(Wire.available())
    dado = Wire.read();
  delay(100);
  return dado;
}

//Funcao de varredura do teclado matricial
char varredura(){
  int i, j;
  for(i=0; i<4; i++){
    digitalWrite(linha[0], HIGH);
    digitalWrite(linha[1], HIGH);
    digitalWrite(linha[2], HIGH);
    digitalWrite(linha[3], HIGH);
    digitalWrite(linha[i], LOW);
    for(j=0; j<3; j++){
      if(digitalRead(coluna[j]) == LOW){
        return teclado[i][j];
      }
    }
    digitalWrite(linha[i], HIGH);
  }
}

//Rotinas de interrupcao

//RSI de evento da UART
void serialEvent() {
  char c;
  while (Serial.available()) {
    c = Serial.read();
    if (c=='\n') {
      insere_Sbuffer('\0'); // Se recebeu um fim de linha, coloca um terminador de string no buffer
      flag_leu_string = 1;
    }
    else {
     insere_Sbuffer(c);
    }
  }
}

//Interrupcao periodica
void RSI_timer(){
  if(flag_deboucing == 0){ //Nao estou em deboucing
    char c = varredura();
    //Verifica se c e um caractere valido
    if((c=='0')||(c=='1')||(c=='2')||(c=='3')||(c=='4')||(c=='5')||(c=='6')||(c=='7')||(c=='8')||(c=='9')||(c=='*')||(c=='#')){
      insere_Tbuffer(c);
      flag_deboucing = 1;
      tempo_deboucing = 500;
    }
  }
  else{ //Estou em deboucing
    tempo_deboucing--;
    if(tempo_deboucing == 0)
      flag_deboucing = 0;
  }
  
}

void setup() {
  //Configura pinos como entrada/saida
  pinMode(LM35, INPUT);
  pinMode(C1, INPUT);
  pinMode(C2, INPUT);
  pinMode(C3, INPUT);
  pinMode(L1, OUTPUT);
  pinMode(L2, OUTPUT);
  pinMode(L3, OUTPUT);
  pinMode(L4, OUTPUT);
  pinMode(LED, OUTPUT);
  
  Timer1.initialize(1000); //Configura timer com periodo 1ms (1kHz)
  Timer1.attachInterrupt(RSI_timer); //Associa a interrupcao periodica a funcao RSI_timer
  
  limpa_Sbuffer(); //Limpa buffer
  
  //Inicia flags
  flag_leu_string = 0;
  flag_deboucing = 0;
  
  delay(500);
  Serial.begin(9600); //Inicia comunicacao serial
  Wire.begin(); //Inicia comunicacao I2C com EEPROM
}

void loop() {
  char saida_buffer[15];
  int flag_escrita = 0;
  
  if (flag_leu_string == 1) {
    if (compara_string(buffer_serial.data, "PING", 4) ) {
      sprintf(saida_buffer, "PONG\n");
      flag_escrita = 1;
    }

    if (compara_string(buffer_serial.data, "ID", 2) ) {
      sprintf(saida_buffer, "NOSSO DATALOGGER\n");
      flag_escrita = 1;
    }
    
    if (compara_string(buffer_serial.data, "MEASURE", 7) ) {
      sprintf(saida_buffer, "%u\n", le_temperatura());
      flag_escrita = 1;
    }
    if (compara_string(buffer_serial.data, "MEMSTATUS", 9) ) {
      int memstatus = le_byte(END_EEPROM, 0x00);
      sprintf(saida_buffer, "%u\n", memstatus);
      flag_escrita = 1;
    }
    if (compara_string(buffer_serial.data, "RESET", 5) ) {
      escreve_byte(END_EEPROM, 0x00, 0x00);
      sprintf(saida_buffer, "Memoria resetada!\n");
      flag_escrita = 1;
    }
    if (compara_string(buffer_serial.data, "RECORD", 6) ) {
      int N = le_byte(END_EEPROM, 0x00);
      int dado = le_temperatura();
      escreve_byte(END_EEPROM, (N+1), byte(dado));
      N++;
      escreve_byte(END_EEPROM, 0x00, byte(N));
      sprintf(saida_buffer, "Dado gravado!\n");
      flag_escrita = 1;
    }
    if (compara_string(buffer_serial.data, "GET", 3) ) {
      int N=0;
      sscanf(buffer_serial.data, "%*s %d", &N);
      int dado = le_byte(END_EEPROM, N);
      sprintf(saida_buffer, "%d\n", dado);
      flag_escrita = 1;
    }
    
    flag_leu_string = 0;
  }
  
  if(flag_leu_teclado == 1){
    if (compara_string(buffer_teclado.data, "#1*", 3) ) {
      for(int i=0; i<6; i++){
        if(i%2){
          digitalWrite(LED,HIGH);
          delay(100);
        }
        else{
          digitalWrite(LED,LOW);
          delay(100);
        }
      digitalWrite(LED,LOW);
      }
    }
    if (compara_string(buffer_teclado.data, "#2*", 3) ) {
      int N = le_byte(END_EEPROM, 0x00);
      int dado = le_temperatura();
      escreve_byte(END_EEPROM, (N+1), byte(dado));
      N++;
      escreve_byte(END_EEPROM, 0x00, byte(N));
      sprintf(saida_buffer, "Dado gravado!\n");
      flag_escrita = 1;
    }
    if (compara_string(buffer_teclado.data, "#3*", 3) ) {
      Serial.println("teste3");
    }
    if (compara_string(buffer_teclado.data, "#4*", 3) ) {
      Serial.println("teste4");
    }

    limpa_Tbuffer();
    flag_leu_teclado = 0;
  }
  
  /* Posso construir uma dessas estruturas if(flag) para cada funcionalidade
   *  do sistema. Nesta a seguir, flag_escrita e habilitada sempre que alguma outra
   *  funcionalidade criou uma requisicao por escrever o conteudo do buffer na
   *  saida UART.
   */
   
  if (flag_escrita == 1) {
    Serial.write(saida_buffer);
    limpa_Sbuffer();
    flag_escrita = 0;
  }

}