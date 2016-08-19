/* DomoChris.fr
 * Projet Timer, déclenchement relais a X jours d'intervale + heure précise (heure creuse par exemple) 
 * Module RTC pour la date + ecran tm 8 digites avec 8 boutons
 * 2 relais
 * 
 * v1 et v2 : mise ne place rtc et calcul date
 * v3 : 23/06/2016 mise en forme
 * V9 ::04/07/2016 rafraichissement uniquement lorsque c'est necessaire
 * v11: 17/08/2016 mise au propre
 * 
 * branchement:
 * tm1638 ecran/bt: STB=>D7, CLK=>D9, DI0=>D8
 * RTC: SDA=>A4, SCL=>A5
 */

// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"
//Ecran Tm1638
#include <TM1638.h>
//Memoire Eeprom Arduino
#include <EEPROM.h> 

#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
   #define Serial SerialUSB
#endif

#define Relais 3       //patte relais

RTC_DS1307 rtc;

// define a afficheur on data pin 8, clock pin 9 and strobe pin 7 Afficheur
TM1638 afficheur(8, 9, 7);

//variables 
           //char daysOfTheWeek[7][12] = {"Di", "Lu", "Ma", "Me", "Je", "Ve", "Sa"};

           byte debug = false; // mode debug true ou false
           byte ClearLed = true; //rafraichissement led au démarrage
           
           int Xjours ; // nombres de jours avant déclenchement paramétrage
           int XjoursDeclenchement; //calcul du nombre de jour restant avant déclenchement
           int XheuresDeclenchement; //calcul du nombre d'heures restant avant déclenchement
           int XminDeclenchement; //calcul du nombre de minutes restant avant déclenchement
           
           byte Declenchement = true; //Flag pour calcul de la prochaine date de déclenchement ce fait au démarrage et lors de changement parametre
           DateTime future; //pour calcul date future
           DateTime now; //pour calcul date now

           int Adeclenchement; //année du prochain déclechement
           byte Mdeclenchement; // Mois du prochain déclechement
           byte Jdeclenchement; //Jour du prochain déclechement
           byte Hdeclenchement; //Heure du prochain déclechement
           byte Mideclenchement; //Minutes du prochain déclechement
           
           byte Hnow; //Heure actuel
           byte Minow; //Minute actuel
           int Anow; //année actuel
           byte Mnow; //Mois actuel
           byte Jnow; //jour actuel
           byte HnowOld; //Heure enregistré pour rafraichissement affichage
           byte MinowOld; //Minute enregistré pour rafraichissement affichage

          //byte Bt; //Quel bouton est appuié
           byte Menu = 0; //quel menu 0=> normal, 1=>nb jour déclenchement + heure declenchement, 2=> reglage heure, 3=> reglage date 4=>sauvegarde dans eeprom
           byte MenuFlag = 1; //Flag menu pour raffraichissement ecran standard
           byte FlagManuel = false; //flag Bt Manuel
           
           //Eeprom
           long XjoursEeprom; //stockage du nombre de jour entre chaque déclenchement, adresse 0
           long HdeclenchementEeprom; //stockage de l'heure entre chaque déclenchement, adresse 4
           long MideclenchementEeprom; //stockage des minutes entre chaque déclenchement, adresse 8

void setup () {

               #ifndef ESP8266
                 while (!Serial); // for Leonardo/Micro/Zero
               #endif

               
               Serial.begin(9600); //moniteur serie
               if (! rtc.begin()) {
                                  Serial.println("Couldn't find RTC");
                                  while (1);
                                   }

                 if (! rtc.isrunning()) {
                                         Serial.println("RTC is NOT running!");
                                         // following line sets the RTC to the date & time this sketch was compiled
                                         rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
                                         // This line sets the RTC with an explicit date & time, for example to set
                                         // January 21, 2014 at 3am you would call:
                                         // rtc.adjust(DateTime(2016, 6, 21, 14, 31, 30));
                                         }
               
                  pinMode(Relais, OUTPUT);   //patte relais sortie
                   digitalWrite(Relais,HIGH); //mise a 0 du relais

                  
                  //lecteur Eeprom
                  Xjours = EEPROMReadlong(0);
                  Hdeclenchement = EEPROMReadlong(4);
                  Mideclenchement = EEPROMReadlong(8);


                  //démarrage affichage
                  afficheur.clearDisplay(); 
                  delay(300);
                  afficheur.setDisplayToString("HELLO");
                  delay(1000);

                  //afficheur.setLEDs(0b11111111 | 0b00000000<< 8 );
                  afficheur.setLEDs(0b00000001);
                  delay(100);
                  afficheur.setLEDs(0b00000010);
                  delay(100);
                  afficheur.setLEDs(0b00000100);
                  delay(100);
                  afficheur.setLEDs(0b00001000);
                  delay(100);
                  afficheur.setLEDs(0b00010000);
                  delay(100);
                  afficheur.setLEDs(0b00100000);
                  delay(100);
                  afficheur.setLEDs(0b01000000);
                  delay(100);
                  afficheur.setLEDs(0b10000000);
                  delay(100);
                  afficheur.setLEDs(0b00000000);
                  delay(100);
                  afficheur.clearDisplay(); 
                  delay(300);
                  //afficheur.setDisplayToString("     LEO");
                  afficheur.setDisplayToString("  PIErrE");
                  delay(1000);
                  
                  afficheur.clearDisplay(); 
                  delay(300);
                  afficheur.setLEDs(0b11111111 | 0b00000000>> 8 );
                  

   
}





void loop () {
        
    
    RTC_Timer(); //input Declenchement ; Xjours, ouput Adeclenchement ; Mdeclenchement ; Jdeclenchement ; Hdeclenchement ,calcul date du prochain déclechement
    Ecran(); //rafraichie ecran en fonction deu Menu
    Bouton(); //gere Bouton Menu
    Eeprom(); //Sauvegarde datetime dans eeprom  
    Fonctionnement(); //gere les relais et déclenchement                  
    Debug(); //debug
    
    
                      
}


void RTC_Timer(){ 
  
  /* variable Declenchement ; Adeclenchement ; Mdeclenchement ; Jdeclenchement ; Hdeclenchement; Xjours
   *  
   * Declenchement : type input, valeur true ou false, déclenche le calcul de la future date, type flag
   * Adeclenchement  : type Output, année de déclenchement format date YYYY variable de type int
   * Mdeclenchement  : type Output, Mois de déclenchement format date MM variable de type byte
   * Jdeclenchement  : type Output, Jour de déclenchement format date JJ variable de type byte
   * Hdeclenchement  : type Output, Heure de déclenchement format date HH variable de type byte
   * Xjours  : type input, nombre de jour avant le prochain declenchement, type byte, limité a 255 jours
   * 
   * library : #include <Wire.h> et #include "RTClib.h"
   */
                                   
  if (Menu == 0){ //DateTime raffraichit que dans le menu principal

    if (Declenchement == true){// calculate a date which is X days and 0 seconds into the future
    now = rtc.now(); //lecture de la date dans le RTC
    
    //Calacul date future
    DateTime futuretmp = now + TimeSpan(Xjours, 0, 0, 0); // rajout xjours 
    DateTime futuretmp2(futuretmp.year(),futuretmp.month(),futuretmp.day(),Hdeclenchement,Mideclenchement,0);
    future = futuretmp2;
    
    Declenchement = false; //remise a 0 du flag une fois le calcul fait
                                 
    }

    now = rtc.now(); //lecture de la date dans le RTC

    XjoursDeclenchement = ((future.unixtime() / 86400L) - (now.unixtime() / 86400L)); //nombre jour restant avant déclanchement
    XheuresDeclenchement = ((future.unixtime() / 3600L) - (now.unixtime() / 3600L)); //nombre d'heure restant avant déclanchement
    XminDeclenchement = ((future.unixtime() / 60L) - (now.unixtime() / 60L)); //nombre de minutes restant avant déclanchement
     
    //année / mois / jours / heure / minutes actuel
    Anow = now.year(), DEC;
    Mnow = now.month(), DEC;
    Jnow = now.day(), DEC;
    Hnow = now.hour(), DEC;
    Minow = now.minute(), DEC;
    }
 
}

void Ecran(){
    /*Fonction de rafraichicement de l'ecran en fonction des Menu, des changement heure / minutes et des paramétrages modifiés
     * 
     */
     
    if (ClearLed == true){ //uniquement au démmarage
    afficheur.setLEDs(0b00000000); //retire toutes les led allumé au démarrage => petit afficheur tm1638
    ClearLed = false;
    }
     
    if ( (Menu == 0)&((HnowOld != Hnow)|| (MinowOld !=Minow) || (MenuFlag == 1))){ //Rafraichit l'affichage en cas de changement d'heure ou changement parametre, ecran standard

    char M0[6] = {0};
    sprintf(M0,"%02u%02u",now.hour(),now.minute());
    char M02[5] = {0};

        if (XjoursDeclenchement > 0){ //affiche jour restant avant declenchement
        sprintf(M02,"J%03u",XjoursDeclenchement);
        }
        if (XheuresDeclenchement < 24){//affiche minute restant avant declenchement quand moins de 24h
        sprintf(M02,"H-%02u",XheuresDeclenchement);
        }
        if ((XminDeclenchement < 99)){//affiche minute restant avant declenchement quand moins de 99 minutes
        sprintf(M02,"M-%02u",XminDeclenchement);
        }
        if ((XminDeclenchement <= 0)){//affiche mode chargement
        sprintf(M02,"CHAR");
        }

        if (FlagManuel == true){ //mode mannuel
        sprintf(M02,"MANU");
        }
        
        strcat(M0, M02); //concatene les deux tableaux
        afficheur.setDisplayToString(M0, 0x40); //affiche 
        HnowOld = Hnow;
        MinowOld = Minow;
        MenuFlag = 0; //raz flag Menu
        
     }
    
      if (Menu == 1){//reglage x jours et heure déclenchement
      
      char M1[6] = {0};
      sprintf(M1,"J%03u",Xjours);
      char M12[5] = {0};
      sprintf(M12,"%02u%02u",Hdeclenchement,Mideclenchement);
      
      strcat(M1, M12);
      afficheur.setDisplayToString(M1, 0x84);
      
      HnowOld = now.hour(); //pour modif 
      MinowOld = now.minute();
      }

    if (Menu == 2){ //réglage heure
      char M2[8] = {0};
      sprintf(M2, "   H%02u%02u", HnowOld,MinowOld );
      afficheur.setDisplayToString(M2, 0x04);   
     }
    if (Menu == 3){ //réglage date
      char M3[8] = {0};
      sprintf(M3,"%02u%02u%02u%02u",Jnow,Mnow,Anow);
      afficheur.setDisplayToString(M3, 0x00);
      MenuFlag = 1; //pour raffraichier ecran au passage a l'ecran standard
          
     }
                      
    
}

void Bouton(){
  /*Fonction bouton
   * Suivant le menu de l'ecran
   * Menu 0: standar, bt actif:  bt 7 change menu, bt8 manuel 
   * Menu 1: chg xjour declenchement et heure declenchement
   * Menu 2: Maj heure RTC
   * Menu 3: Maj date RTC
   */
  
 
  byte keys = afficheur.getButtons(); //Li bouton
  // light the 8 red LEDs as the buttons are pressed
  if ((keys<128)&(keys>0)){ //affiche led bouton appuyé, la 8eme led qui est gerer dans Fonctionnement
  afficheur.setLEDs(keys); 
  delay(300);
  afficheur.setLEDs(0b00000000); //raz led sauf la 8 eme led
   }
  
  if (keys == 1){//Bouton 1

    if (Menu == 1){ //réglage Jour -
     Xjours = Xjours -1;
            
      if (Xjours == -1){ //sous le 0
        Xjours = 999; //affichage max
      }
    }
    
    if (Menu == 2){ //réglage Heure -
      HnowOld = HnowOld- 1;
      
      if (HnowOld == 255){ //sous le 0
        HnowOld = 23;
      }
    }

    if (Menu == 3){ //Réglage jour date -
      Jnow = Jnow - 1;
      
      if (Jnow == 255){ //sous le 0
        Jnow = 31;
      }
    }
    
  }
  
  if (keys == 2){//Bouton 2
    if (Menu == 1){ //réglage Jour +
     Xjours = Xjours + 1;
    
      if (Xjours == 1000){ //max affichage, limite a 999 jours
        Xjours = 0; //raz
      }
    }
    
    if (Menu == 2){ //réglage Heure +
      HnowOld = HnowOld+ 1;
       if (HnowOld >23){
        HnowOld = 0;
      }
    }

    if (Menu == 3){ //Réglage jour date +
      Jnow = Jnow + 1;
      if (Jnow == 32){ //max jour
        Jnow = 1;
      }
    }
    
  }
  
  if (keys == 4){ //Bouton 3
     if (Menu == 1){ //réglage Heure -
     Hdeclenchement = Hdeclenchement - 1;
    
      if (Hdeclenchement == 255){ //max affichage
        Hdeclenchement = 23; //raz
      }
    }
    
    
   if (Menu == 2){ //reglage Minute -
      MinowOld = MinowOld- 1;
      if (MinowOld == 255){
        MinowOld = 59;
      }
    }

     if (Menu == 3){ //Réglage mois date -
      Mnow = Mnow - 1;
      if (Mnow == 255){ //max mois
        Mnow = 12;
      }
    }
  }
  
  if (keys == 8){//Bouton 4
     if (Menu == 1){ //réglage Heure +
     Hdeclenchement = Hdeclenchement + 1;
    
      if (Hdeclenchement == 24){ //max affichage
        Hdeclenchement = 0; //raz
      }
    }
    
   if (Menu == 2){ //reglage Minute +
      MinowOld = MinowOld+ 1;
      if (MinowOld >59){
        MinowOld = 0;
      }
    }

    if (Menu == 3){ //Réglage mois date +
      Mnow = Mnow + 1;
      if (Mnow == 13){ //max mois
        Mnow = 1;
      }
    }
  }
  
  if (keys == 16){//Bouton 5
     if (Menu == 1){ //réglage Minute -
     Mideclenchement = Mideclenchement - 1;
    
      if (Mideclenchement == 255){ //max affichage
        Mideclenchement = 59; //raz
      }
    }

    if (Menu == 3){ //Réglage annee date -
      Anow = Anow - 1;
      
    }
   
  }
  if (keys == 32){//Bouton 6
    
  if (Menu == 1){ //réglage Minute +
     Mideclenchement = Mideclenchement + 1;
    
      if (Mideclenchement == 60){ //max affichage
        Mideclenchement = 0; //raz
      }
    }

    if (Menu == 3){ //Réglage annee date -
      Anow = Anow + 1;
      
    }
   
  }
  
  if (keys == 64){ //Bouton 7 Bt Menu 
    
    Menu = (Menu +1);
       if (Menu > 4){ //3 menu + 4eme pour sauvegarde eeprom et rafraichissement calcul date future
        Menu = 0;
       }
       delay(300);//delais tempon 
       
  }
  
  if (keys == 128){ //Bt 8 marche forcé manuel on /off
         digitalWrite(Relais,(1-(digitalRead(Relais))) );
    //   Bt = 0; //raz valeur Bouton une fois fonction effectué
       delay(300);//delais tempon 
       FlagManuel = (1-digitalRead(Relais)); //Flag pour Fonctionnement
       
        MenuFlag = true; //rafraichissement ecran
       if (digitalRead(Relais) == LOW){ //cacul date déclenchement apres le mode mannuel
        Declenchement = true;
       }
       
  }
  
}

void EEPROMWritelong(int address, long value) //Ecriture dans EEprom
      {
      //Decomposition from a long to 4 bytes by using bitshift.
      //One = Most significant -> Four = Least significant byte
      byte four = (value & 0xFF);
      byte three = ((value >> 8) & 0xFF);
      byte two = ((value >> 16) & 0xFF);
      byte one = ((value >> 24) & 0xFF);

      //Write the 4 bytes into the eeprom memory.
      EEPROM.write(address, four);
      EEPROM.write(address + 1, three);
      EEPROM.write(address + 2, two);
      EEPROM.write(address + 3, one);
      }

//This function will return a 4 byte (32bit) long from the eeprom
//at the specified address to address + 3.

long EEPROMReadlong(long address) //Lecture Eeprom
      {
      //Read the 4 bytes from the eeprom memory.
      long four = EEPROM.read(address);
      long three = EEPROM.read(address + 1);
      long two = EEPROM.read(address + 2);
      long one = EEPROM.read(address + 3);

      //Return the recomposed long by using bitshift.
      return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
      }


void Eeprom(){
  /*Sauvegarde datetime dans eeprom rtc + Xjour, Hdeclenchement, Mideclenchement dans eeprom Arduino
   * La sauvegarde ce fait en Menu 4 (aprés cycle de modif)
   */
 

  now = rtc.now(); //lecture de la date dans le RTC

            
  if ((Menu ==4)&((Xjours != EEPROMReadlong(0))||(EEPROMReadlong(4)!=Hdeclenchement)||(EEPROMReadlong(8)!=Mideclenchement)|| (Anow != now.year())||( Mnow != now.month()) || (Jnow != now.day())||(Hnow != HnowOld)||(Minow != MinowOld))){//force raffraichissement calcul declenchement
    Declenchement = true; //recalcul date aprés modif
  } 
  
  if ((Menu ==4)&((EEPROMReadlong(0)!=Xjours)||(EEPROMReadlong(4)!=Hdeclenchement)||(EEPROMReadlong(8)!=Mideclenchement))){ //sauvegarde aprés le menu 3 si differences Xjours, Hdeclenchement, Mideclenchement dans eeprom Arduino
  EEPROMWritelong(0, Xjours);
  EEPROMWritelong(4, Hdeclenchement);
  EEPROMWritelong(8, Mideclenchement);
  }
  
  if(Menu == 4){ //sauvegarde aprés le menu 3 si differences Heure minutes JJ/MM/AAAA
  
   if ((Hnow != HnowOld)||(Minow != MinowOld)|| ((now.year(), DEC) != Anow) || ((now.month(), DEC) != Mnow) || ((now.day(), DEC) != Jnow )){
   rtc.adjust(DateTime(Anow, Mnow, Jnow, HnowOld, MinowOld, 00));
   }
  }
  if(Menu == 4){
  Menu  = 0; //aprés sauvegarde et rafraichissement retour au menu standard
  }
  
}

void Fonctionnement(){ //Fonction pour déclencher relais 
                                 
  if (FlagManuel == false){   //si pas de forcage manuel
                                    
    if (XminDeclenchement>0){ //Timer pas finis
    digitalWrite(Relais, HIGH); //raz relais
    }

    if (XminDeclenchement<=0){ //Timer finis activation relais
    digitalWrite(Relais, LOW);
    }
                                  
    if (XminDeclenchement <= -480){ //Timer finis durée du déclenchement relais fixé a 8H
    digitalWrite(Relais, HIGH); //raz relais
    Declenchement = true; //Calcul nouvelle date a la fin 
    }
  }


  //Allume led bouton lorsque relais actif
                                  
  if (digitalRead(Relais) == HIGH){ //lumiere allumé pour relais, led 8 bt
  afficheur.setLEDs(0b00000000);
  }
  else
  afficheur.setLEDs(0b10000000);
}

                                  

   
  
  void Debug(){ //debug
     if (debug == true){
                      //declenchement
                      Serial.print(" maintenant + ");
                      Serial.print (Xjours);
                      Serial.print ( " Jours a : ");
                      Serial.print(Hdeclenchement);
                      Serial.println(":00 h : ");

                      Serial.print("Date de declenchement loops : ");
                      Serial.print(Adeclenchement);
                      Serial.print('/');
                      Serial.print(Mdeclenchement);
                      Serial.print('/');
                      Serial.print(Jdeclenchement);
                      Serial.print('/');
                      Serial.print(Hdeclenchement);
                      Serial.print(":00");
                      Serial.println();

                       Serial.print("Date/heure now : ");
                       Serial.print(Anow);
                       Serial.print("/");
                       Serial.print(Mnow);
                       Serial.print("/");
                       Serial.print(Jnow);
                       Serial.print("h:");
                       Serial.print(Hnow);
                       Serial.print(':');
                       Serial.println(Minow);
                      }  
  
}




