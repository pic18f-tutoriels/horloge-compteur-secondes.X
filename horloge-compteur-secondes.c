/*
 * File:   interruptions.c
 * Author: jmgonet
 *
 * Created on 19. novembre 2012, 08:21
 */

#include <xc.h>
#include <stdio.h>


/**
 * Bits de configuration:
 */
#pragma config FOSC = INTIO67  // Oscillateur interne, ports A6 et A7 comme IO.
#pragma config IESO = OFF      // Pas d'embrouilles avec l'osc. au démarrage.
#pragma config FCMEN = OFF     // Pas de monitorage de l'oscillateur.

#pragma config MCLRE = INTMCLR // Pas de Master Reset. RE3 est un port IO.s

/**
 * Fonction qui transmet un caractère à la EUSART.
 * Il s'agit de l'implémentation d'une fonction système qui est
 * appelée par <code>printf</code>.
 * Si un terminal est connecté aux sorties RX / TX, il affichera du texte.
 * @param data Le code ASCII du caractère è afficher.
 */
void putch(char data) {
    while( ! TXIF) {
        continue;
    }
    TXREG = data;
}

/**
 * Configuration de la EUSART comme sortie asynchrone � 1200 bauds.
 * On assume que le PIC18 fonctionne � Fosc = 1MHz.
 */
void EUSART_Initialize() {
    // Pour une fréquence de 1MHz, ceci donne 1200 bauds:
    SPBRG = 12;
    SPBRGH = 0;

    // Configure RC6 et RC7 comme entrées digitales, pour que
    // la EUSART puisse en prendre le contrôle:
    TRISCbits.RC6 = 1;
    TRISCbits.RC7 = 1;

    // Configure la EUSART:
    RCSTAbits.SPEN = 1;  // Active la EUSART.
    TXSTAbits.SYNC = 0;  // Mode asynchrone.
    TXSTAbits.TXEN = 1;  // Active l'�metteur.
}

/**
 * Table de conversion pour afficheur LED 7 segments.
 */
unsigned char AFFICHAGE_led[] = {0x3f,0x06,0x5b,0x4f,
                                 0x66,0x6d,0x7d,0x07,
                                 0x7f,0x6f,0x77,0x7c,
                                 0x39,0x5e,0x79,0x71};

/**
 * Contenu à afficher sur l'afficheur 7 segments multiple.
 */
unsigned char AFFICHAGE_contenu[7];

/**
 * Prochain digit à activer.
 */
unsigned char AFFICHAGE_digit = 0;

/**
 * À chaque appel, affiche un digit sur l'afficheur
 * multiple. Cette fonction est à appeler successivement
 * pour obtenir l'affichage de tous les digits.
 */
void AFFICHAGE_raffraichir()
{
    unsigned char a,c;

    c = 1 << AFFICHAGE_digit;
    a = AFFICHAGE_contenu[AFFICHAGE_digit] - '0';
    a = ~AFFICHAGE_led[a];

    PORTC = 0;
    PORTA = a;
    PORTC = c;

    if(AFFICHAGE_digit++>5)
    {
        AFFICHAGE_digit=0;
    }
}

/**
 * Compteur pour les secondes de l'horloge.
 */
unsigned char HORLOGE_secondes;

/**
 * Compteur pour les minutes de l'horloge.
 */
unsigned char HORLOGE_minutes;

/**
 * Compteur pour les heures de l'horloge.
 */
unsigned char HORLOGE_heures;

/**
 * Actualise la chaîne de caractères à afficher
 */
void HORLOGE_compte() {
    if (HORLOGE_secondes++ > 59) {
        HORLOGE_secondes = 0;
        if (HORLOGE_minutes++ > 59) {
            HORLOGE_minutes = 0;
        }
        if (HORLOGE_heures++ > 12) {
            HORLOGE_heures = 0;
        }
    }
    sprintf(AFFICHAGE_contenu,"%02d%02d%02d", HORLOGE_heures,
            HORLOGE_minutes, HORLOGE_secondes);
}

/**
 * Compte les interruptions du temporisateur, pour
 * mesurer les secondes.
 */
unsigned char tics=0;

/**
 * Routine des interruptions.
 */
void interrupt interruptions() {
    if (INTCONbits.TMR0IF) {
        // Baisse le drapeau d'interruption pour la prochaine fois.
        INTCONbits.TMR0IF = 0;

        // Établit le compteur du temporisateur 0 pour un débordement
        // dans 6.67ms:
        TMR0H = 0xF9;
        TMR0L = 0x7D;
        AFFICHAGE_raffraichir();
        if (tics++>=150) {
            tics=0;
            HORLOGE_compte();
        }
    }
}

/**
 * Point d'entrée.
 * Configure les périphériques puis laisse la routine
 * d'interruptions faire le travail.
 */
void main() {
    EUSART_Initialize();

    // Pas de convertisseur A/D sur cette application:
    ANSELA = 0;
    ANSELB = 0;
    ANSELC = 0;

    // Configure le port A et les 6 LSB du port C comme sorties digitales,
    // pour pouvoir contrôler l'afficheur.
    TRISA = 0;

    TRISCbits.RC0 = 0;
    TRISCbits.RC1 = 0;
    TRISCbits.RC2 = 0;
    TRISCbits.RC3 = 0;
    TRISCbits.RC4 = 0;
    TRISCbits.RC5 = 0;

    // Configure le temporisateur 0 pour obtenir 150 interruptions par seconde,
    // en assumant que le microprocesseur est cadencé à 1MHz
    T0CONbits.TMR0ON = 1;  // Active le temporisateur 0.
    T0CONbits.T08BIT = 0;  // 16 bits pour compter jusqu'à 3125.
    T0CONbits.T0CS = 0;    // On utilise Fosc/4 comme source.
    T0CONbits.PSA = 1;     // On n'utilise pas le diviseur de fréquence.

    // Active les interruptions de haute priorité:
    RCONbits.IPEN = 1;
    INTCONbits.GIEH = 1;
    INTCONbits.GIEL = 0;    // Désactive les interruptions de basse priorité.

    // Active les interruptions pour le temporisateur 0:
    INTCONbits.TMR0IE = 1;

    // Laisse la routine d'interruptions faire le travail.
    while(1);
}
