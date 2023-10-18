#include "stm32f10x.h"

/*****************************************************************
Preambule : indiquez ici les periopheriques que vous avez utilisez
*****************************************************************/

// GPIOA   : broche 6 pour controler la LED verte
// GPIOC : broche 13 pour d?tecter l'appui du bouton
// TIM2 : chronometre les 300 ms
// TIM3 : chronometre l'intervalle de temps avant d'aller la LED
// TIM4 : assure le clignotement de la led pour la victoire
// EXTI13 : provoque l'interruption sur un front du bouton


/*****************************************************************
Declaration des fonctions
*****************************************************************/
int rand(void);
void configure_gpio_pa5(void) ;
void configure_gpio_pc13(void) ;
void set_gpio(GPIO_TypeDef *GPIO, int n) ;
void reset_gpio(GPIO_TypeDef *GPIO, int n) ;
void configure_timer(TIM_TypeDef *TIM, int psc, int arr) ;
void configure_it(void) ;
void start_timer(TIM_TypeDef *TIM) ;
void stop_timer(TIM_TypeDef *TIM) ;
void configure_afio_exti_pc13(void);

/*****************************************************************
Varibales globales
 *****************************************************************/

int led_on = 0;
int victoire = 0;

/*****************************************************************
MAIN
*****************************************************************/

int main(void){
	
    // Configuration des ports d'entree/sortie
	configure_gpio_pa5();
	configure_gpio_pc13();
	configure_afio_exti_pc13();
    
    // Configuration des timers
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM4EN ;
	configure_timer(TIM2, 7199, 2999);
	configure_timer(TIM3, 7199, 10*rand());
	configure_timer(TIM4, 7199, 2499);
    
    // Configuration des interruptions
	configure_it();
    
    // Demarrage du permier timer
  start_timer(TIM3);
    
    // Boucle d'attente du processeur
  while (1){
	}
    
	return 0;
}

/*****************************************************************
Corps des fonctions
*****************************************************************/

/**
Configure la broche 5 du port A (led verte)
*/
void configure_gpio_pa5(void){
	// Activation de l'horloge de GPIOA
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	// Ouput push-pull : 0b0001
	GPIOA->CRL &= ~(0xF << 20);
	GPIOA->CRL |= (0x01 << 20);
}

/**
Configure la broche 13 du port C (bouton USER) 
*/
void configure_gpio_pc13(void) {
	// Activation de l'horloge de GPIOC
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
	// Input floating 0b0100
	GPIOC->CRH &= ~(0xF << 20);
	GPIOC->CRH |= (0x01 << 22 );
}


/**
Met a 1 la sortie de la broche n du port GPIO
*/
void set_gpio(GPIO_TypeDef *GPIO, int n) {
	GPIO->ODR |= (0x01 << n);
}

/**
Met a 0 la sortie de la broche n du port GPIO
*/
void reset_gpio(GPIO_TypeDef *GPIO, int n) {
		GPIO->ODR &= ~(0x01 << n);
}

/**
Configure la periode du timer TIM en fonction des parametres
psc (prescaler) et arr (autoreload) sans lancer le timer
*/
void configure_timer(TIM_TypeDef *TIM, int psc, int arr) {
	TIM->ARR = arr;
	TIM->PSC = psc;
}

/**
Demarre le timer TIM
*/
void start_timer(TIM_TypeDef *TIM) {
	TIM->CR1 |= 0x01;
}

/**
Arrete le timer TIM
*/
void stop_timer(TIM_TypeDef *TIM) {
	TIM->CR1 &= ~0x01;
}

/**
Configure toutes les interruptions du systeme
*/
void configure_it(void) {
	// Interruption du timer 2
	TIM2->DIER |= TIM_DIER_UIE; // validation de l'it de TIM2
	NVIC->ISER[0] |= NVIC_ISER_SETENA_28 ; // validation de l'IT 28 (TIM2) dans le NVIC
	// Interruption du timer 3
	TIM3->DIER |= TIM_DIER_UIE; // validation de l'it de TIM3
	NVIC->ISER[0] |= NVIC_ISER_SETENA_29 ; // validation de l'IT 29 (TIM3) dans le NVIC
	// Interruption du timer 4
	TIM4->DIER |= TIM_DIER_UIE; // validation de l'it de TIM4
	NVIC->ISER[0] |= NVIC_ISER_SETENA_30 ; // validation de l'IT 30 (TIM4) dans le NVIC
	// Configure EXTI13
	EXTI->IMR |= (0x01 << 13); // Validatin de l'it externe des ports 13
	EXTI->FTSR |= (0x01 << 13); // Leve une it sur front descendant
	NVIC->ISER[1] |= NVIC_ISER_SETENA_8 ; // validation de l'IT 40 (EXTI10_15) dans le NVIC

}

/**
Configure le port PC13 comme source d'interruption externe
*/
void configure_afio_exti_pc13(void) {
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
	AFIO->EXTICR[3] &= ~(0x0F << 4);
	AFIO->EXTICR[3] |= (0x02 << 4);
}

/*****************************************************************
Fonctions d'interruption
*****************************************************************/

void TIM2_IRQHandler(void){
	reset_gpio(GPIOA, 5);
	led_on = 0;
	stop_timer(TIM2);
	configure_timer(TIM3, 7199, 10*rand());
	start_timer(TIM3);
	TIM2->SR &= ~TIM_SR_UIF ;
}

void TIM3_IRQHandler(void){
	set_gpio(GPIOA, 5);
	led_on = 1;
	stop_timer(TIM3);
	start_timer(TIM2);
	TIM3->SR &= ~TIM_SR_UIF ;
}

void TIM4_IRQHandler(void){
	if(GPIOA->ODR & (0x01 << 5)) {
		reset_gpio(GPIOA,5);
	} else {
		set_gpio(GPIOA, 5);
	}
		
	TIM4->SR &= ~TIM_SR_UIF ;
}

void EXTI15_10_IRQHandler(void){
	if (led_on){
		victoire = 1;
		stop_timer(TIM2);
		stop_timer(TIM3);
		start_timer(TIM4);
	}
	EXTI->PR |= (0x01 << 13);
}

/*****************************************************************
Fonctions pre-definies
*****************************************************************/

/**
Retourne une valeur entiere aleatoire comprise entre 800 et 1800
*/
int rand(){
	static int randomseed = 0;
	randomseed = (randomseed * 9301 + 49297) % 233280;
	return 800 + (randomseed % 1000);
}

