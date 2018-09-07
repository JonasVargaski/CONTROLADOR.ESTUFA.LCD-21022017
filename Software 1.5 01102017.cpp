/* # MODIFICAÇOES --->> Adicionado wdt ao software para resetar caso os firmware trave.
                        Modificado setup para iniciar todos os pinos desligados.
                        Alterado no setup função lcd.write para lcd.print() no ajuste de relogio estava acusando erro
                        Arruma tabela de umidade automatica que nao alterava abaixo da minima definida

*/
// # PROJETO CONTROLADOR ESTUFA
#include <avr/wdt.h>
#include <wordtostr.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include "RTClib.h"
//Configuraçao do Dysplay//

LiquidCrystal lcd(44, 42, 52, 50, 48, 46);
RTC_DS1307 rtc;

/*
pino 1 VSS = GND
pino 2 VDD = 5v
pino 3 V0  = POTENCIOMETRO
pino 4 RS =  48 no arduino             //22           ;;;;44
pino 5 R/W =  GND
pino 6 E  =  49 no arduino             //21          ;;;;42
pino 7 D0 = db0 (dados)               
pino 8 D1 = db1 (dados)               
pino 9 D2 = db2 (dados)               
pino 10 D3 = db3 (dados)               
pino 11 D4 = db4 pino 50 no arduino (dados) //11       ;;;;52
pino 12 D5 = db5 pino 51 no arduino (dados) //12       ;;; 50
pino 13 D6 = db6 pino 52 no arduino (dados) //13       ;;; 48         
pino 14 D7 = db7 pino 53 no arduino (dados) //14       ;;;; 46
pino 15 A = +5v (led)
pino 16 K = GND (led)
 */

////////// DEFINIÇÃO DOS PINOS atmega 2560 /////////////////
#define ventuinha_RL1 2 // 19  Nomral pino 2, tem que arrumar transistor.
#define flap_RL1 3 //24
#define flap_RL2 4  //26
#define abafador_RL1 5  // 16
#define abafador_RL2 6// 25
#define buzzer 7 //14

#define on HIGH
#define off LOW

#define sensor_temperatura A0  // 38
#define sensor_umidade A1     // 39

//#BOTOES TELA ////
#define b_ok 24        // 2          ;;;;24   
#define b_menu 32      //6           ;;;;32
#define b_menos 26     //3           ;;;;26
#define b_mais 30       //5          ;;;;30
#define b_cima 28      //4           ;;;;;28
#define b_baixo 22    //1            ;;;;22  

#define in_power   12 // pino 22 como sensor para falta de energia; (VERIFICAR TRANSISTOR BC635, SUBSTITUIR POR BC337);

String versao_firmware ="1.03";
DateTime rtcc;

//# DECLARAÇÃO DAS VARIAVIES  #//

byte x, y, ok, menu,clear, temp_ajst, umid_ajst, alarm_ajst, aciona_flap, contador, alarme_auto,tempo_vent, inicia_tempo_flap, tempo_info, alarme_status, liga_alarme, timer_alarme, modo_ajst, habilita_ventoinha,temp_calibra , umid_calibra, info, tempo_menu, ref_leitura;
String temperatura_string, umidade_string, temp_ajst_string, umid_ajst_string;
char buffer[10];
float temp, umid, ref_umid, ref_temp ;
int tempo_estagio_flap , rearma_alarme;

void setup() {
    wdt_disable();//inicia o watchdog  desligado ,caso o mesmo tenha se resetado

	 Wire.begin();
     rtc.begin();
     lcd.begin(20, 4);
    
    pinMode(ventuinha_RL1, OUTPUT);
    pinMode(flap_RL1, OUTPUT);
    pinMode(flap_RL2, OUTPUT);
    pinMode(abafador_RL1, OUTPUT);
    pinMode(abafador_RL2, OUTPUT);
    pinMode(buzzer, OUTPUT);
    pinMode(sensor_temperatura, INPUT);
    pinMode(sensor_umidade, INPUT);
    pinMode(b_ok, INPUT);
    pinMode(b_menu, INPUT);
    pinMode(b_baixo, INPUT);
    pinMode(b_cima, INPUT);
    pinMode(b_mais, INPUT);
    pinMode(b_menos, INPUT);


    digitalWrite(ventuinha_RL1, LOW);
    digitalWrite(flap_RL1, LOW);
    digitalWrite(flap_RL2, LOW);
    digitalWrite(abafador_RL1, LOW);
    digitalWrite(abafador_RL2, LOW);
    digitalWrite(buzzer, LOW);


    temp_ajst = EEPROM.read(0);
    umid_ajst = EEPROM.read(1);
    alarme_auto = EEPROM.read(2);
    modo_ajst = EEPROM.read(3);
    temp_calibra = EEPROM.read(4);
    umid_calibra = EEPROM.read(5);
    
    lcd.setCursor(0, 0);
    lcd.print("#CONTROLADOR CR500 #");
    delay(1500);
    lcd.setCursor(0, 3);
    lcd.print("Iniciando...");
    delay(2500);
    if ((digitalRead(b_menu)) == off) {
        info = 10;
        menu_instalador();
    }
     while (!rtc.isrunning()) {
    lcd.print("Relogio Parado, pressione OK para ajustar!");
    delay(80);
    if((digitalRead(b_ok)) == off){
     	lcd.clear();
    	delay(100);
        relogio();
    }
    
  }


    if ((digitalRead(b_menos) == off) && (digitalRead(b_mais)) == off){
      lcd.setCursor(0,0);
      lcd.print("Resetando           Parametros          Aguarde...");
      delay(12000);
      temp_calibra = 0.0;
        umid_calibra = 0.0;
        temp_ajst = 85;
        umid_ajst = 85;
        alarme_auto = 0;
        modo_ajst = 0;

        EEPROM.write(0, temp_ajst);
        EEPROM.write(1, umid_ajst);
        EEPROM.write(2, alarme_auto);
        EEPROM.write(3, modo_ajst);
        EEPROM.write(4, temp_calibra);
        EEPROM.write(5, umid_calibra);
    }

    void leitura();
    void timer();
    void controle();
    void teclado();
    void alarme();
    void menu_informacao();
    void menu_temperatura();
    void menu_umidade();
    void salva_cfg();
    void menu_menu();
    void menu_modo_trabalho();
    void temp_calibra();
    void umid_calibra();
    void relogio();
    
}
//**********************************************************************************************************************

void loop() {
    wdt_enable(WDTO_2S);        // liga WDT em 2 segundos;


    while (menu == 0) { //&& analogRead(in_power) != 0) { 
        wdt_enable(WDTO_2S);
        leitura();
        timer();
        controle();
        teclado();
        alarme();
        menu_informacao();
        wdt_reset();       
    }
    // ####### MENU ####### 
    while (menu == 10) {
       
        menu_menu();
        wdt_disable();
    }
    while (menu == 5) {
        salva_cfg();
        timer();
        wdt_disable();
    }
}
//**********************************************************************************************************************
void relogio(){
    
     byte X,posi=0;
     	int ano,mes,dia,hora,minuto,segundo,segundo_backup ;
     	ano=rtcc.year();
     	mes=rtcc.month();
     	dia=rtcc.day();
     	hora=rtcc.hour();
     	minuto=rtcc.minute();
     	segundo=rtcc.second();

     while(ok==2 ){
     	rtcc=rtc.now();     		
     		if(segundo_backup!= rtcc.second()){
     		lcd.clear();
     	    segundo_backup=rtcc.second();
     	}
     	    if((digitalRead(b_mais)) == off){
			posi++;
			delay(300);
			}
			if((digitalRead(b_menos)) == off){
			posi--;
			delay(300);
			}
			if((digitalRead(b_ok)) == off){
			ok=3;
			delay(500);
			lcd.clear();
			} 
			if(posi>6 || posi<0){
				posi=0;
			}
     	if(posi==0 ){
       		lcd.setCursor(0,0);
     		lcd.print(" Ajuste de Relogio  ");
     		lcd.setCursor(6,1);
    		lcd.print(rtcc.day(), DEC);
    		lcd.print('/');
    		lcd.print(rtcc.month(), DEC);
    		lcd.print('/');
    		lcd.print(rtcc.year(), DEC);
    		lcd.setCursor(6,2);
    		lcd.print(rtcc.hour(), DEC);
    		lcd.print(':');
    		lcd.print(rtcc.minute(), DEC);
    		lcd.print(':');
    		lcd.print(rtcc.second(), DEC);
    		lcd.setCursor(0,3);
    		lcd.print("<<                >>");
		} 
		if(posi == 1){
		lcd.setCursor(0,0);
     		lcd.print(" Ajuste de *ANO ");
     		lcd.setCursor(9,2);
    		lcd.print(ano);
    		lcd.setCursor(0,3);
    		lcd.print("<<                >>");
		if((digitalRead(b_cima)) == off){
			ano++;
			delay(250);
		}   
		if((digitalRead(b_baixo)) == off){
			ano--;
			delay(250);
		} 
	}
	if(posi == 2){
		lcd.setCursor(0,0);
     		lcd.print(" Ajuste de *MES ");
     		lcd.setCursor(9,2);
    		lcd.print(mes);
    		lcd.setCursor(0,3);
    		lcd.print("<<                >>");
		if((digitalRead(b_cima)) == off){
			mes++;
			delay(250);
		}   
		if((digitalRead(b_baixo)) == off){
			mes--;
			delay(250);
		} 
	}
	if(posi == 3){
		lcd.setCursor(0,0);
     		lcd.print(" Ajuste de *DIA ");
     		lcd.setCursor(9,2);
    		lcd.print(dia);
    		lcd.setCursor(0,3);
    		lcd.print("<<                >>");
		if((digitalRead(b_cima)) == off){
			dia++;
			delay(250);
		}   
		if((digitalRead(b_baixo)) == off){
			dia--;
			delay(250);
		} 
	}
	if(posi == 4){
		lcd.setCursor(0,0);
     		lcd.print(" Ajuste de *HORA ");
     		lcd.setCursor(9,2);
    		lcd.print(hora);
    		lcd.setCursor(0,3);
    		lcd.print("<<                >>");
		if((digitalRead(b_cima)) == off){
			hora++;
			delay(250);
		}   
		if((digitalRead(b_baixo)) == off){
			hora--;
			delay(250);
		} 
	}
	if(posi == 5){
		lcd.setCursor(0,0);
     		lcd.print(" Ajuste de *MINUTO ");
     		lcd.setCursor(9,2);
    		lcd.print(minuto);
    		lcd.setCursor(0,3);
    		lcd.print("<<                >>");
		if((digitalRead(b_cima)) == off){
			minuto++;
			delay(250);
		}   
		if((digitalRead(b_baixo)) == off){
			minuto--;
			delay(250);
		} 
	}
	if(posi == 6 && ok==2){
		lcd.setCursor(0,0);
     		lcd.print(" SALVAR AJUSTES ? ");
     		lcd.setCursor(6,1);
    		lcd.print(dia);
    		lcd.print('/');
    		lcd.print(mes);
    		lcd.print('/');
    		lcd.print(ano);
    		lcd.setCursor(6,2);
    		lcd.print(hora);
    		lcd.print(':');
    		lcd.print(minuto);
    		lcd.print(':');
    		lcd.print(segundo);
    		lcd.setCursor(0,3);
    		lcd.print("<<                >>");
    		if((digitalRead(b_ok)) == off){
			rtc.adjust(DateTime(ano,mes,dia,hora,minuto,segundo));
			lcd.clear();
			lcd.setCursor(0,0);
     		lcd.print(" SALVO! ");
			delay(2000);
			posi=0;
	}

}
		

     }
}

//**********************************************************************************************************************

void menu_menu() {

    if (y < 1 || y > 7 ) {
        y = 1;
    }
    if (ok < 1 || ok > 2) {
        ok = 1;
    }

    if (tempo_menu >= 200 && menu == 10) { // Sai automatico do menu após 1.40 minutos.
        menu = 0;

        // lcd.clear();
        lcd.setCursor(1, 1);
        lcd.print("AJUSTES DESCARTADOS");
        delay(800);
    }

         if(ok!=clear){
      lcd.clear();
      clear = ok;
        }
    //lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("# PARAMETRIZACAO #");

 

    if (y == 1 && ok == 1 && menu == 10) {
        lcd.setCursor(1, 1);
        lcd.print("Ajuste TEMPERATURA  ");
        lcd.setCursor(0, 3);
        lcd.print("OK ENTRA | MENU SAIR");
        }    
    if (y == 1 && ok == 2 && menu == 10) {
        menu_temperatura();
    }    
    if (y == 2 && ok == 1 && menu == 10) {
        lcd.setCursor(1, 1);
        lcd.print("Ajuste UMIDADE      ");
        lcd.setCursor(0, 3);
        lcd.print("OK ENTRA | MENU SAIR");
    }
    if (y == 2 && ok == 2 && menu == 10) {
        menu_umidade();
    }
    if (y == 3 && ok == 1 && menu == 10) {
        lcd.setCursor(0, 1);
        lcd.print(" Ajuste MODO UMIDADE");
        lcd.setCursor(0, 3);
        lcd.print("OK ENTRA | MENU SAIR");
    }
    if (y == 3 && ok == 2 && menu == 10) {
        menu_modo_trabalho();
    }
    if (y == 4 && ok == 1 && menu == 10) {
        lcd.setCursor(1, 1);
        lcd.print("Ajuste MODO ALARME  ");
        lcd.setCursor(0, 3);
        lcd.print("OK ENTRA | MENU SAIR");
    }
    if (y == 4 && ok == 2 && menu == 10) {
        menu_alarme();
    }
    if (y == 5 && ok == 1 && menu == 10) {
        lcd.setCursor(1, 1);
        lcd.print("Ajuste SENSOR TEMP  ");
        lcd.setCursor(0, 3);
        lcd.print("OK ENTRA | MENU SAIR");
    }
    if (y == 5 && ok == 2 && menu == 10) {
        menu_calibra_temp();
    }
     if (y == 6 && ok == 1 && menu == 10) {
        lcd.setCursor(1, 1);
        lcd.print("Ajuste SENSOR UMID  ");
        lcd.setCursor(0, 3);
        lcd.print("OK ENTRA | MENU SAIR");
    }
    if (y == 6 && ok == 2 && menu == 10) {
        menu_calibra_umid();
    }

    teclado();
    delay(180);


    if (y == 7 && ok == 1 && menu == 10) {
        lcd.setCursor(1, 1);
        lcd.print("Ajuste RELOGIO      ");
        lcd.setCursor(0, 3);
        lcd.print("OK ENTRA | MENU SAIR");
    }
    if (y == 7 && ok == 2 && menu == 10) {
        relogio();
    }

    teclado();
    delay(180);

    if (menu == 10 && (digitalRead(b_menu)) == off) {
        menu = 5;
        lcd.clear();
        lcd.print("Aguarde...");
        delay(800);
        lcd.clear();
    }
}
//**********************************************************************************************************************    

void salva_cfg() {

    if (tempo_menu == 240 && menu == 5) { // Sai automatico do menu após 2 minutos.
        menu = 0;
    }
    
    lcd.setCursor(2, 1);
    lcd.print("SALVAR AJUSTES ?");
    lcd.setCursor(0, 3);
    lcd.print("(OK) SIM  NAO (MENU)");

    if (menu == 5 && (digitalRead(b_ok)) == off) {
        EEPROM.write(0, temp_ajst);
        EEPROM.write(1, umid_ajst);
        EEPROM.write(2, alarme_auto);
        EEPROM.write(3, modo_ajst);
        EEPROM.write(4, temp_calibra);
        EEPROM.write(5, umid_calibra);
        menu = 0;
        lcd.clear();
        lcd.setCursor(1, 1);
        lcd.print("AJUSTES SALVOS !!!");
        delay(800);
    }
    if (menu == 5 && (digitalRead(b_menu)) == off) {
        temp_ajst = EEPROM.read(0);
        umid_ajst = EEPROM.read(1);
        alarme_auto = EEPROM.read(2);
        modo_ajst = EEPROM.read(3);
        temp_calibra = EEPROM.read(4);
        umid_calibra = EEPROM.read(5);
        menu = 0;
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("AJUSTES DESCARTADOS");
        delay(800);
    }
}
//**********************************************************************************************************************

void teclado() {

    if ((digitalRead(b_baixo)) == off) {
        y++;
    }
    if ((digitalRead(b_cima)) == off) {
        y--;
    }
    if ((digitalRead(b_mais)) == off) {
        x++;
    }
    if ((digitalRead(b_menos)) == off) {
        x--;
    }
    if ((digitalRead(b_menu) == off) && menu == 0) {
        delay(350);
        lcd.clear();
        menu = 10;
        tempo_menu = 0;
    }
    if ((digitalRead(b_ok)) == off && menu == 10) {
        ok++;
    }
    if ((digitalRead(b_ok)) == off && menu == 0 && habilita_ventoinha == 1) {
        habilita_ventoinha = 0;
        delay(1000);
    }
    if ((digitalRead(b_ok)) == off && menu == 0 && habilita_ventoinha == 0) {
        habilita_ventoinha = 1;
        delay(1000);
    }
    
}
//**********************************************************************************************************************

void timer() {
	rtcc = rtc.now();

    delay(500);
    contador++;
    timer_alarme++;
    tempo_menu++;
    tempo_info++;
    tempo_vent++;

    if (tempo_menu == 243) { //2,150 minutos ;
        tempo_menu = 0;
    }
    if (contador == 10) { // 5 segundos ;
        contador = 0;
    }
    if (timer_alarme == 3) {
        timer_alarme = 0;
    }
    if (tempo_info == 30) { // 15 segundos;
        tempo_info = 0;
    }
    
}
//**********************************************************************************************************************

void leitura() {

    if (ref_leitura < 2) {
        for(int i=0;i<20;i++){
        ref_temp += (temp_calibra +(analogRead(sensor_temperatura) * 0.9 + 32));
        ref_umid += (umid_calibra +(analogRead(sensor_umidade) * 0.9 + 32));
        }
         ref_leitura++;
    } else {
        temp = ref_temp / 40;
        umid = ref_umid / 40;
        
        ref_temp = 0;
        ref_umid = 0;
        ref_leitura = 0;
    }

    if (temp > 50) {
        temperatura_string = wordtostr(buffer, temp, 0);
        temp_ajst_string = wordtostr(buffer, temp_ajst, 0);
        lcd.setCursor(0, 0);
        lcd.print("TEMPERATURA:        "); //+ temperatura_string + " /" + temp_ajst_string );
        lcd.setCursor(12, 0);
        lcd.print(temperatura_string + " /" + temp_ajst_string);
    } else {
        lcd.setCursor(0, 0);
        lcd.print("Falha sensor temp   ");
    }

    if (umid > 50) {
        umidade_string = wordtostr(buffer, umid, 0);
        umid_ajst_string = wordtostr(buffer, umid_ajst, 0);
        lcd.setCursor(0, 1);
        lcd.print("UMIDADE:            "); // + umidade_string + " /" + umid_ajst_string );
        lcd.setCursor(9, 1);
        lcd.print(umidade_string + " /" + umid_ajst_string);
    } else {
        lcd.setCursor(0, 1);
        lcd.print("Falha sensor umid   ");
    }
}
//**********************************************************************************************************************

void menu_modo_trabalho() {

    if (modo_ajst < 1) {
        modo_ajst = 1;
    } else if (modo_ajst > 2) {
        modo_ajst = 2;
    }

    //lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Controle de Umidade");
    lcd.setCursor(0, 3);
    if (modo_ajst == 2) {
        lcd.print("# AUTOMATICO ");
    } else {
        lcd.print("# MANUAL ");
    }
    lcd.setCursor(1, 1);
    lcd.print(" (-)MANUAL AUTO(+)");


    if ((digitalRead(b_mais)) == off) {
        modo_ajst++;
    }
    if ((digitalRead(b_menos)) == off) {
        modo_ajst--;
    }
    if ((digitalRead(b_ok)) == off) {
        ok = 3;
        delay(100);
    }
}

//**********************************************************************************************************************

void menu_temperatura() {


    if (temp_ajst < 85) {
        temp_ajst = 85;
    }
    if (temp_ajst > 165) {
        temp_ajst = 165;
    }

    //lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Ajuste Temperatura");
    lcd.setCursor(0, 3);
    lcd.print("PROG :     ");
    lcd.setCursor(3, 1);
    lcd.print("<< - TEMP + >>");
    lcd.setCursor(9, 3);
    lcd.print(temp_ajst);

    if ((digitalRead(b_mais)) == off) {
        temp_ajst++;
        //delay(100);
    }
    if ((digitalRead(b_menos)) == off) {
        temp_ajst--;
        //delay(100);
    }
    if ((digitalRead(b_ok)) == off) {
        ok = 3;
        delay(100);
    }
}
//**********************************************************************************************************************

void menu_umidade() {

    if (umid_ajst < 80) {
        umid_ajst = 80;
    }
    if (umid_ajst > 120) {
        umid_ajst = 120;
    }

   // lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Ajuste Umidade");
    lcd.setCursor(3, 1);
    lcd.print("<< - UMID + >>");
    lcd.setCursor(0, 3);
    lcd.print("PROG : ");
    lcd.setCursor(9, 3);
    lcd.print(umid_ajst);

    if ((digitalRead(b_mais)) == off) {
        umid_ajst++;
    }
    if ((digitalRead(b_menos)) == off) {
        umid_ajst--;
    }
    if ((digitalRead(b_ok)) == off) {
        ok = 3;
        delay(100);
    }
}
//**********************************************************************************************************************

void menu_calibra_temp(){
    float temp_lida = (analogRead(sensor_temperatura) * 0.9 + 32);
    leitura();    
    //lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ajuste Sensor temp  ");
    lcd.setCursor(0, 1);
    lcd.print("< (- 1 ) SP (+ 1 ) >");
    lcd.setCursor(0,2);
    lcd.print("ATUAL:");
    lcd.setCursor(8,2);
    lcd.print(temp_lida);
    lcd.setCursor(0, 3);
    lcd.print("PROG : ");
    lcd.setCursor(7, 3);
    lcd.print(temp_calibra);
    lcd.setCursor(14,3);
    lcd.print(temp);

    if ((digitalRead(b_mais)) == off) {
        temp_calibra ++;
    }
    if ((digitalRead(b_menos)) == off) {
        temp_calibra --;
    }
    if ((digitalRead(b_ok)) == off) {
        ok = 3;
        delay(100);
    }
}

//**********************************************************************************************************************
void menu_calibra_umid(){
    float umid_lida = (analogRead(sensor_umidade) * 0.9 + 32);
    leitura();
    //lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ajuste Sensor umid  ");
    lcd.setCursor(0, 1);
    lcd.print("< (- 1 ) SP (+ 1 ) >");
    lcd.setCursor(0,2);
    lcd.print("ATUAL:");
    lcd.setCursor(8,2);
    lcd.print(umid_lida);
    lcd.setCursor(0, 3);
    lcd.print("PROG : ");
    lcd.setCursor(7, 3);
    lcd.print(umid_calibra);
    lcd.setCursor(14,3);
    lcd.print(umid);    

    if ((digitalRead(b_mais)) == off) {
        umid_calibra ++;
    }
    if ((digitalRead(b_menos)) == off) {
        umid_calibra --;
    }
    if ((digitalRead(b_ok)) == off) {
        ok = 3;
        delay(100);
    }
}

//**********************************************************************************************************************
void umidade_automatica() {

    if (temp_ajst <= 90 | temp_ajst == 91) {
        umid_ajst = 88;
    } else
        if (temp_ajst == 92 | temp_ajst == 93) {
        umid_ajst = 90;
    } else
        if (temp_ajst == 94 | temp_ajst == 95) {
        umid_ajst = 92;
    } else
        if (temp_ajst == 96 | temp_ajst == 97) {
        umid_ajst = 94;
    } else
        if (temp_ajst == 98 | temp_ajst == 99) {
        umid_ajst = 96;
    } else
        if (temp_ajst == 100 | temp_ajst == 101) {
        umid_ajst = 97;
    } else
        if (temp_ajst >= 102 && temp_ajst <= 117) {
        umid_ajst = 96;
    } else
        if (temp_ajst == 118 | temp_ajst == 119) {
        umid_ajst = 97;
    } else
        if (temp_ajst == 120 | temp_ajst == 122) {
        umid_ajst = 98;
    } else
        if (temp_ajst == 123 | temp_ajst == 124) {
        umid_ajst = 99;
    } else
        if (temp_ajst == 125 | temp_ajst == 126) {
        umid_ajst = 100;
    } else
        if (temp_ajst == 127 | temp_ajst == 128) {
        umid_ajst = 101;
    } else
        if (temp_ajst == 129 | temp_ajst == 130) {
        umid_ajst = 102;
    } else
        if (temp_ajst == 131 | temp_ajst == 132) {
        umid_ajst = 103;
    } else
        if (temp_ajst >= 133 && temp_ajst <= 148) {
        umid_ajst = 104;
    } else
        if (temp_ajst >= 149 && temp_ajst <= 152) {
        umid_ajst = 105;
    } else
        if (temp_ajst == 153 | temp_ajst == 154) {
        umid_ajst = 106;
    } else
        if (temp_ajst == 155 | temp_ajst == 156) {
        umid_ajst = 108;
    } else
        if (temp_ajst == 157 | temp_ajst == 158) {
        umid_ajst = 109;
    } else
        if (temp_ajst == 159) {
        umid_ajst = 110;
    } else
        if (temp_ajst >= 160) {
        umid_ajst = 112;
    }
}

//**********************************************************************************************************************
void menu_alarme() {
  while (ok == 2){

    if ((digitalRead(b_mais)) == off && y == 4) {
        alarme_auto = 1;
    }
    if ((digitalRead(b_menos)) == off && y == 4) {
        alarme_auto = 0;
    }
    if ((digitalRead(b_ok)) == off) {
        ok = 3;
        delay(100);
    }
    //lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Ajuste Alarme       ");
    lcd.setCursor(0, 1);
    lcd.print("(-) MANUAL| AUTO (+)");

    if (alarme_auto == 1) {
        lcd.setCursor(0, 3);
        lcd.print("#Alarme  Automatico ");
    }
    else if(alarme_auto == 0) {
        lcd.setCursor(0, 3);
        lcd.print("#Alarme Manual      ");
      }
    }
}
//**********************************************************************************************************************

void menu_informacao() {
    if (tempo_info <= 4) {

        if (alarme_status == 1) {
            lcd.setCursor(0, 3);
            lcd.print("# Alarme  Ligado    ");
        }
        if (alarme_status == 0) {
            digitalWrite(buzzer, off);
            lcd.setCursor(0, 3);
            lcd.print("# Alarme Desligado  ");
        }
    }
    if (tempo_info > 4 && tempo_info <= 8) {
        if (modo_ajst == 2) {
            lcd.setCursor(0, 3);
            lcd.print("# Umidade Automatica");
        } else {
            lcd.setCursor(0, 3);
            lcd.print("# Umidade Manual    ");
        }
    }
    if (tempo_info > 8 && tempo_info <= 12) {
        if (habilita_ventoinha == 1) {
            lcd.setCursor(0, 3);
            lcd.print("# Ventoinha Ligada  ");
        } else if (habilita_ventoinha == 0) {
            lcd.setCursor(0, 3);
            lcd.print("# Ventoinha Deslig  ");
        }
    }
    if(alarme_auto == 1 && tempo_info >= 13 && tempo_info <= 17 && alarme_status == 0){  // Alarme religa se modo estiver em automatico 7 minutos após ser delsigado
      int tempo_restante = (420 - (rearma_alarme/2));
      String info_alarme = wordtostr(buffer, tempo_restante, 0);
      lcd.setCursor(0, 3);
        lcd.print("# Alarme on :" + info_alarme + " sec");
    }
    else if (tempo_info > 12) {
        tempo_info = 0;
    }

}

//**********************************************************************************************************************

void menu_instalador() {
    while (info == 10) {
        lcd.setCursor(0, 0);
        lcd.print(" Liga       saidas !");
        lcd.setCursor(0, 1);
        lcd.print("DESENVOLVIDO POR   :");
        lcd.setCursor(0, 2);
        lcd.print("JONAS VARGASKI      ");
        lcd.setCursor(0, 3);
        lcd.print("VERSAO Firmware:" + versao_firmware);
        delay(400);
        
        // Liga todas as Saidas

        digitalWrite(ventuinha_RL1, on);
        digitalWrite(flap_RL1, on);
        digitalWrite(flap_RL2, off);
        digitalWrite(abafador_RL1, on);
        digitalWrite(abafador_RL2, off);
        digitalWrite(buzzer, on);
        delay(3000);

        if ((digitalRead(b_ok)) == off) {
        info = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Desliga     saidas!");
        lcd.setCursor(0, 1);
        lcd.print("DESENVOLVIDO POR   :");
        lcd.setCursor(0, 2);
        lcd.print("JONAS VARGASKI      ");
        lcd.setCursor(0, 3);
        lcd.print("VERSAO Firmware:" + versao_firmware);
        delay(400);
        
        // Desliga todas as Saidas

        digitalWrite(ventuinha_RL1, off);
        digitalWrite(flap_RL1, off);
        digitalWrite(flap_RL2, on);
        digitalWrite(abafador_RL1, off);
        digitalWrite(abafador_RL2, on);
        digitalWrite(buzzer, off);
        delay(8000);
        lcd.clear();
        if ((digitalRead(b_ok)) == off) {
            info = 0;
            lcd.clear();
        }
        }
    }
}

//**********************************************************************************************************************

void controle() {
    if (modo_ajst == 2) { //teste se for modo automatico;
        umidade_automatica();
    }

    if (temp > 50) {
        // CONTROLE DA VENTOINHA
        if (habilita_ventoinha == 0) {
            digitalWrite(ventuinha_RL1, off);
            tempo_vent++;
            if(tempo_vent ==230 && habilita_ventoinha == 0){
              habilita_ventoinha = 1;
              tempo_vent=0;
            }
            if (tempo_vent == 231){
            tempo_vent = 0;
            } 
        }
        if (temp + 1 < temp_ajst && habilita_ventoinha == 1 && contador > 6) {
            digitalWrite(ventuinha_RL1, on);
        }
        if (temp >= temp_ajst && habilita_ventoinha == 1 && contador > 6) {
            digitalWrite(ventuinha_RL1, off);
        }

        // CONTROLE ABAFADOR
        if (temp < temp_ajst && contador > 6) {
            digitalWrite(abafador_RL1, on);
            digitalWrite(abafador_RL2, off);
        } 
        if ((temp == temp_ajst && contador > 6 ) | (temp > temp_ajst && contador > 6)) {
            digitalWrite(abafador_RL1, off);
            digitalWrite(abafador_RL2, on);
        }
    } else {
        digitalWrite(ventuinha_RL1, off);
        digitalWrite(abafador_RL1, off);
        digitalWrite(abafador_RL2, on);
    }

    // CONTROLE DE FLAP COM ESTAGIOS; //////
    if (umid > 50) {

        if (umid < umid_ajst && contador > 6) {
            inicia_tempo_flap = 1;
        } else
            if ((umid == umid_ajst && aciona_flap == 0 && contador > 6) | (umid > umid_ajst && aciona_flap == 0 && contador > 6)) {
            tempo_estagio_flap = 0;
            inicia_tempo_flap = 0;
            digitalWrite(flap_RL1, off);
            digitalWrite(flap_RL2, on);
        }
        if (inicia_tempo_flap == 1) {
            tempo_estagio_flap++;

            if (tempo_estagio_flap >= 1 && tempo_estagio_flap <= 4) { // 2 segundos aciona
                aciona_flap = 1;
            } else
                if (tempo_estagio_flap >= 5 && tempo_estagio_flap <= 35) { // 15 segundos parado
                aciona_flap = 0;
            } else
                if (tempo_estagio_flap >= 36 && tempo_estagio_flap <= 40) { // 2 segundos aciona
                aciona_flap = 1;
            } else
                if (tempo_estagio_flap >= 41 && tempo_estagio_flap <= 71) {// 15 segundos parado
                aciona_flap = 0;
            } else
                if (tempo_estagio_flap >= 72 && tempo_estagio_flap <= 77) {// 2 segundos aciona
                aciona_flap = 1;
            } else
                if (tempo_estagio_flap >= 78 && tempo_estagio_flap <= 108) {// 15 segundos parado
                aciona_flap = 0;
            } else
                if (tempo_estagio_flap > 108) {
                aciona_flap = 1; // MANTEN ACIONADO
            } else
                if (tempo_estagio_flap > 1800 && temp + 5 <= temp_ajst) { // Se ficar 15 minutos flap aberto e temperatura estiver 5 graus abaixo do programado, fecha flap até temperatura subir. 
                aciona_flap = 0;
                tempo_estagio_flap = 1850;
            }
        } else {
            aciona_flap = 0;
        }
        if (aciona_flap == 1) {
            digitalWrite(flap_RL1, on);
            digitalWrite(flap_RL2, off);
        } else
            if (aciona_flap == 0) {
            digitalWrite(flap_RL1, off);
            digitalWrite(flap_RL2, on);
        }
    } else {
        digitalWrite(flap_RL1, off);
        digitalWrite(flap_RL2, off);
    }
}
//**********************************************************************************************************************

void alarme() {
    if(alarme_auto == 1){
      if(alarme_status != 1){     // Se tiver desligado e em modo automatico, começa a contar tempo de rearme.
      rearma_alarme ++;
    }else {
      rearma_alarme = 0;
    }
      if(rearma_alarme > 840){
        alarme_status = 1;
        rearma_alarme = 0;
      }
    }
    
    if (menu == 0 && alarme_status == 0 && (digitalRead(b_cima)) == off) {
        alarme_status = 1;
    }
    if (menu == 0 && alarme_status == 1 && (digitalRead(b_baixo)) == off) {
        alarme_status = 0;
    }
    if(temp < 50){
       lcd.setCursor(1, 2);
        lcd.print("                    "); 
    }
    if ((temp + 6) < temp_ajst && (temp > 50)) {
        liga_alarme = 1;
        lcd.setCursor(0, 2);
        lcd.print(" TEMPERATURA   BAIXA");
        
    } else if ((temp - 5) > temp_ajst) {
        liga_alarme = 1;
        lcd.setCursor(0, 2);
        lcd.print(" TEMPERATURA   ALTA");
    } else {
        liga_alarme = 0;
        lcd.setCursor(1, 2);
        lcd.print("                   ");
    }
   if (alarme_status == 1 && liga_alarme == 1 && timer_alarme == 2) {
        digitalWrite(buzzer, on);
    delay(60);
      digitalWrite(buzzer, off);
    } 
  }
