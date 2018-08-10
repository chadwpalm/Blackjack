/*
   Casino Blackjack v. 1.0
   Created by Chad Palmer
   (Insert open source info here)

   This code requires a hardware setup using a 20x4 character LCD screen (a 16x2 was too limiting)
   using an HD44780 chipset controller, (4) tactile buttons (N/O), a piezo speaker (not required),
   and a transistor (not required - explained in the instructable).

   Inclusion of two header files (deck.h and card.h) is required and should sit either in the same
   directory as this file or in your Arduino IDE main library folder.

   This code implements common rules of Blackjack with the following casino style house rules:

   1. Dealer stands on soft 17.
   2. Splitting is allowed, but hitting on split aces is not permitted. The split hand will be at the
      same value as the bet.
   3. Doubling down is allowed (if there is enough funds available) and will be equal to value of bet.
   4. Doubling down on a split is not permitted
   5. Player Blackjacks payout at 3:2.
   6. Insurance can be purchased when dealer's up card shows an ace and is set at 50% of bet value.
   7. There is no surrender option.

   On bootup, player starts with $1000 cash. Betting is done in increments of $10 in order to prevent
   fractions.  When player is out of cash, the game is over and the game must be reset.  At the start
   of the game the player can decide on how many decks will be in the dealer's shoe (1, 2, or 3).
   The cards are shuffled when the shoe reaches 75% of cards used to prevent the shoe from running
   out of cards during a hand.

   The cards are shuffled randomly and the order of the deck is determined at shuffle, not randomly at
   the beginning of each hand.  This produced a genuine experience.  When more than one deck is used,
   the decks are shuffled together, but still retain the proper amount of cards. For example, if using
   two decks the shoe should have (8) aces (two of each suit), (8) K's (two of each suit), etc.  Card
   counting (if you know how to do it) should work as if playing a real game.

   I hope you enjoy the game and I did my best to comment up the code for easier understanding.

   This program compiles at approximately 15K.  It should be small enough to load onto any Arduino. An
   Arduino (or ATMega chip) with 2K of SRAM is recommended. (It might work with 1.5K, but not tested).

   This code can be found on github at:
   An instructable for building the hardward can be found at:
*/

////////////////////////////////////////////////////////////////
//  Ths section defines all the global variables, arrays, and //
//  objects.                                                  //
////////////////////////////////////////////////////////////////

#include "deck.h"               //Deck object library (see deck.h for more details)
#include <LiquidCrystal.h>      //LCD object library (https://www.arduino.cc/en/Reference/LiquidCrystal)

LiquidCrystal lcd(7, 8, 9, 10, 11, 12); //Create lcd object and set pins (see instructable)

//Deck objects are created empty of cards
deck shoe;    //Create deck of cards object that will go in the dealer's shoe
deck player;  //Create a set of cards object that will be the player's hand
deck dealer;  //Create a set of cards object that will be the dealer's hand
deck split1;  //Create a set of cards object that will be the first set of cards if player splits
deck split2;  //Create a set of cards object that will be the second set of cards if player splits

//////////////////////
// Global Variables //
//////////////////////
const byte SPEAKER = 6;     //Speaker Pin
bool has_insurance;         //Flag for if player purchased insurance
bool in_round = true;       //Flag for if round is in session
bool player_turn = true;    //Flag for if the player is taking his turn
bool dealer_turn = true;    //Flag for if the dealer is taking his turn
bool double_down;           //Flag for if player has doubled down
bool split;                 //Flag for if player has split his hand
byte split_hands = 0;       //Variable stating how many hands didn't bust for score keeping
byte handResult = 0;        //Variable that states the result of the hand for the endHand function
byte player_score[2];       //Array that holds the player's hand's score {soft ace score, hard ace score]
byte dealer_score[2];       //Array that holds the dealer's hand's score {soft ace score, hard ace score]
byte split1_score[2];       //Array that holds the player's first split score {soft ace score, hard ace score]
byte split2_score[2];       //Array that holds the player's second split  {soft ace score, hard ace score]
unsigned long bet = 10;     //Variable for bet increment (set to define betting increment)
unsigned long money = 1000; //Variable for total cash to start with in dollars (ex. start game with $1000)

//Set buttons to arduino input pins
byte b_top = A4;
byte b_right = A2;
byte b_left = A5;
byte b_bottom = A3;

//Button state flags. Used for detecting button presses.
bool lastButton_top = LOW;
bool currentButton_top = LOW;
bool lastButton_left = LOW;
bool currentButton_left = LOW;
bool lastButton_right = LOW;
bool currentButton_right = LOW;
bool lastButton_bottom = LOW;
bool currentButton_bottom = LOW;

//This array translates card rank numbers (1 - 13) generated by the card object (ex. Ace = 1, J = 11) to their correct
//card rank (ex. A for aces, K for King, etc.). 10 has a uniquely created character to fit in a single character block.
//Since arrays start at 0, the first variable in the array is not used, so I assigned it as 0.
const char face[14] = {0, 65, 50, 51, 52, 53, 54, 55, 56, 57, 0, 74, 81, 75};

//Create Suit characters to be displayed on the lcd screen during play
//These are custom characters that are stored in the lcd memory.
byte heart[8] =
{
  B01010,
  B11111,
  B11111,
  B11111,
  B11111,
  B01110,
  B01110,
  B00100
};

byte diamond[8] =
{
  B00100,
  B01110,
  B01110,
  B11111,
  B11111,
  B01110,
  B01110,
  B00100
};

byte spade[8] =
{
  B00100,
  B01110,
  B11111,
  B11111,
  B11111,
  B01110,
  B00100,
  B11111
};

byte club[8] =
{
  B00100,
  B01110,
  B00100,
  B01010,
  B11111,
  B01010,
  B00100,
  B11111
};

//Create the ten character
byte ten[8] =
{
  B11000,
  B01000,
  B01000,
  B11100,
  B00010,
  B00101,
  B00101,
  B00010
};

//Create the up and down arrows. (Most LCDs only have left and right arrows)
byte up[8] =
{
  B00000,
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00000,
  B00000
};

byte down[8] =
{
  B00000,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100,
  B00000,
  B00000
};

//////////////////////////////////////////////////////////////////
//  The entirety of the game was created in the setup function. //
//  The loop function is not used.                              //
//////////////////////////////////////////////////////////////////
void setup()
{

  lcd.begin(20, 4); //Initialize LCD for use

  //Burn custom characters into LCD memory
  lcd.createChar(0, ten);
  lcd.createChar(1, heart);
  lcd.createChar(2, diamond);
  lcd.createChar(3, spade);
  lcd.createChar(4, club);
  lcd.createChar(5, up);
  lcd.createChar(6, down);

  //Set the pinmodes for the buttons to utilize pullup resistors
  pinMode(b_top, INPUT_PULLUP);
  pinMode(b_bottom, INPUT_PULLUP);
  pinMode(b_left, INPUT_PULLUP);
  pinMode(b_right, INPUT_PULLUP);

  //Game title screen
  lcd.setCursor(5, 1);
  lcd.print(F("Welcome to"));
  lcd.setCursor(2, 2);
  lcd.print(F("Casino Blackjack"));
  delay(2000);

  /////////////////////////////////////////////////////
  // Code for selecting number of decks to play with //
  /////////////////////////////////////////////////////
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print(F("Select number"));
  lcd.setCursor(0, 2);
  lcd.print(F("of decks?"));
  lcd.setCursor(16, 1);
  lcd.print(F("2"));
  lcd.print(char(5));
  lcd.setCursor(14, 2);
  lcd.print(F("1"));
  lcd.print(char(127));
  lcd.print(F("  3"));
  lcd.print(char(126));

  bool go = false;
  //The program will loop until a button is pressed to determine number of decks
  while (!go)
  {
    currentButton_top = digitalRead(b_top); //read button state
    if (lastButton_top == HIGH && currentButton_top == LOW) //if it was pressed…
    {
      shoe.setDecks(2);
      go = true;
    }
    lastButton_top = currentButton_top; //reset button value

    currentButton_left = digitalRead(b_left); //read button state
    if (lastButton_left == HIGH && currentButton_left == LOW) //if it was pressed…
    {
      shoe.setDecks(1);
      go = true;
    }
    lastButton_left = currentButton_left; //reset button value

    currentButton_right = digitalRead(b_right); //read button state
    if (lastButton_right == HIGH && currentButton_right == LOW) //if it was pressed…
    {
      shoe.setDecks(3);
      go = true;
    }
    lastButton_right = currentButton_right; //reset button value
  }

  /////////////////////////////////////////////////////////////
  // End of introductions and setup. Now lets start the game //
  /////////////////////////////////////////////////////////////

  while (money > 0)   //This is the program root. It ends it's cycle when money is gone.
  {                   
    shoe.shuffle(lcd);  //The shoe is shuffled here.  

    while (shoe.size() >= (52 * shoe.numDecks() * .25) && money > 0) //This block cycles until 25% of
    {                                                                //cards are left, then reshuffles 
      place_bet(bet, money); //Calls function for placing a bet

      initial_deal();        //Calls function to deal the initial hands to player and dealer

      if (money - bet >= bet / 2) //Checks if dealer is showing an ace and asks if player wants to
        insurance();              //purchase insurance against dealer blackjack. The condition to
                                  //run the function is based on having enough cash to cover the insurance bet.
                      
      check_blackjack();          //Calls function to check if either player or dealer has blackjack

      if (in_round == true)       //If neither player has blackjack, hand countinues here.
      {
        delay(1000);

        //Clear out insurance question if it was asked
        lcd.setCursor(7, 2);
        lcd.print(F("             "));
        lcd.setCursor(7, 3);
        lcd.print(F("             "));

        //This block of code displays menu of choices after initial hand is dealt. Double down and split
        //options only appear if there is enough money to cover them (hence the if statements). It then waits
        //for a button press and assigns the variable 'choice'.
        if (money - bet >= bet)
        {
          lcd.setCursor(8, 2);
          lcd.print(F("DbDwn"));
          lcd.print(char(127));
        }
        lcd.setCursor(16, 2);
        lcd.print(F("Hit"));
        lcd.print(char(5));
        lcd.setCursor(15, 3);
        lcd.print(F("Stay"));
        lcd.print(char(6));
        lcd.setCursor(8, 3);
        lcd.print(F("       "));

        if (check_split() && money - bet >= bet)
        {
          lcd.setCursor(8, 3);
          lcd.print(F("Split"));
          lcd.print(char(126));
        }

        go = false;
        byte choice;

        while (!go)
        {
          currentButton_top = digitalRead(b_top); //read button state
          if (lastButton_top == HIGH && currentButton_top == LOW) //if it was pressed…
          {
            choice = 1;
            go = true;
          }
          lastButton_top = currentButton_top; //reset button value

          if (money - bet >= bet)
          {
            currentButton_left = digitalRead(b_left); //read button state
            if (lastButton_left == HIGH && currentButton_left == LOW) //if it was pressed…
            {
              choice = 2;
              go = true;
            }
            lastButton_left = currentButton_left; //reset button value
          }

          currentButton_bottom = digitalRead(b_bottom); //read button state
          if (lastButton_bottom == HIGH && currentButton_bottom == LOW) //if it was pressed…
          {
            choice = 3;
            go = true;
          }
          lastButton_bottom = currentButton_bottom; //reset button value

          if (check_split() && money - bet >= bet)
          {
            currentButton_right = digitalRead(b_right); //read button state
            if (lastButton_right == HIGH && currentButton_right == LOW) //if it was pressed…
            {
              choice = 4;
              go = true;
            }
            lastButton_right = currentButton_right; //reset button value
          }
        }

        //After a button is pressed this next section plays out the actions based on the selection
        if (choice == 4) //Decided to split
        {
          split = true;  //Sets the split flag to true
          
          //This block creates the split decks from the players hand and then deals them out on the screen.
          player.start();
          split1.add_rear(player.current_rank(), player.current_suit());
          player.remove_front();
          player.start();
          split2.add_rear(player.current_rank(), player.current_suit());
          player.remove_front();
          player.start();
          split1.add_rear(shoe.current_rank(), shoe.current_suit());
          shoe.remove_front();
          shoe.start();
          split2.add_rear(shoe.current_rank(), shoe.current_suit());
          shoe.remove_front();
          shoe.start();
          split1.start();
          split2.start();

      //The animation that moves the dealer's up card before the split hands are dealt.
          lcd.clear();
          dealer.start();
          deal_card_up(3, 2, dealer);
          delay(200);
          lcd.clear();
          deal_card_up(1, 2, dealer);
          delay(200);
          lcd.clear();
          deal_card_up(0, 2, dealer);
          lcd.setCursor(0, 0);
          lcd.print(F("P:"));

          deal_card_up(3, 0, split1);
          byte split_rank = split1.current_rank();
          split1.advance();
          delay(500);
          deal_card_up(3, 2, split2);
          split2.advance();
          delay(500);
          deal_card_up(5, 0, split1);
          delay(500);
          deal_card_up(5, 2, split2);
          calculate_total(split1, split1_score);
          calculate_total(split2, split2_score);

          if (split_rank == 1)  //Rules only allow for one card on split aces. If the split cards are aces,
                                //then the option to deal out more cards is bypasses by this if/else statement.
          {
            split_hands += 2;   //Since both split hands cannot be busted, this variable states that there are two
            delay(2000);        //valid non-busted split hands
          } else {
            
            //Not aces, so start with top split hand. This next section deals out the hand for the first split
            //and stops when you either bust or press the button for 'stay'.
            split_hands++;  //This flags that a valid non-busted hand is present (gets removed if hand busts)
            lcd.setCursor(2, 0);
            lcd.print(char(126));
            display_split1_score(15, 0);

            lcd.setCursor(16, 2);
            lcd.print(F("Hit"));
            lcd.print(char(5));
            lcd.setCursor(15, 3);
            lcd.print(F("Stay"));
            lcd.print(char(6));

            go = false;
            byte choice;

            while (!go)
            {
              if (split1_score[1] != 21)
              {
                currentButton_top = digitalRead(b_top); //read button state
                if (lastButton_top == HIGH && currentButton_top == LOW) //if it was pressed…
                {
                  choice = 1;
                  go = true;
                }
                lastButton_top = currentButton_top; //reset button value
              }

              currentButton_bottom = digitalRead(b_bottom); //read button state
              if (lastButton_bottom == HIGH && currentButton_bottom == LOW) //if it was pressed…
              {
                choice = 2;
                go = true;
              }
              lastButton_bottom = currentButton_bottom; //reset button value
            }

            if (choice == 2)
              player_turn = false;

            while (player_turn)
            {
              //This code might be hard to follow.  A card is added to the split1 deck and removed from the shoe.
              //Then the cursor is advanced to the last card in the deck and deals it out onto the screen in the
              //next position.
              split1.add_rear(shoe.current_rank(), shoe.current_suit());
              shoe.remove_front();
              shoe.start();
              split1.start();
              for (byte i = 0; i < split1.size() - 1; i++)
                split1.advance();

              deal_card_up(3 + ((split1.size() - 1) * 2), 0, split1);
              calculate_total(split1, split1_score);

              //After each card is dealt, it must be determined if the hand is a bust. If not, the new score
              //is displayed and the option to hit again or stay is presented.
              if (split1_score[1] > 21)
              {
                lcd.setCursor(15, 0);
                lcd.print(F("Bust!"));
                playTone(2);
                money -= bet;
                split_hands--;  //Here's the bust, so split_hands gets decrimented
                player_turn = false;
                delay(1000);
              } else {
                display_split1_score(15, 0);
                lcd.setCursor(7, 2);
                lcd.print(F("             "));
                lcd.setCursor(7, 3);
                lcd.print(F("             "));

                if (split1_score[1] != 21)  //This if statement is not allowing the hit option if already at 21
                {                           //Who would hit on 21?
                  lcd.setCursor(16, 2);
                  lcd.print(F("Hit"));
                  lcd.print(char(5));
                }
                lcd.setCursor(15, 3);
                lcd.print(F("Stay"));
                lcd.print(char(6));

                go = false;
                byte choice;

                while (!go)
                {
                  if (split1_score[1] != 21)
                  {
                    currentButton_top = digitalRead(b_top); //read button state
                    if (lastButton_top == HIGH && currentButton_top == LOW) //if it was pressed…
                    {
                      choice = 1;
                      go = true;
                    }
                    lastButton_top = currentButton_top; //reset button value
                  }

                  currentButton_bottom = digitalRead(b_bottom); //read button state
                  if (lastButton_bottom == HIGH && currentButton_bottom == LOW) //if it was pressed…
                  {
                    choice = 2;
                    go = true;
                  }
                  lastButton_bottom = currentButton_bottom; //reset button value
                }

                if (choice == 2)
                  player_turn = false;
              }
            }

            ////////////////////////////////////////////////////////////////////////
            // Let's move on to second split hand. Pretty much same as the first. //
            ////////////////////////////////////////////////////////////////////////
            split_hands++;
            player_turn = true;
            lcd.setCursor(7, 2);
            lcd.print(F("             "));
            lcd.setCursor(7, 3);
            lcd.print(F("             "));
            lcd.setCursor(2, 0);
            lcd.print(F(" "));
            lcd.setCursor(2, 2);
            lcd.print(char(126));

            display_split2_score(15, 2);

            lcd.setCursor(15, 0);
            lcd.print(F(" Hit"));
            lcd.print(char(5));
            lcd.setCursor(15, 1);
            lcd.print(F("Stay"));
            lcd.print(char(6));

            go = false;

            while (!go)
            {
              if (split2_score[1] != 21)
              {
                currentButton_top = digitalRead(b_top); //read button state
                if (lastButton_top == HIGH && currentButton_top == LOW) //if it was pressed…
                {
                  choice = 1;
                  go = true;
                }
                lastButton_top = currentButton_top; //reset button value
              }

              currentButton_bottom = digitalRead(b_bottom); //read button state
              if (lastButton_bottom == HIGH && currentButton_bottom == LOW) //if it was pressed…
              {
                choice = 2;
                go = true;
              }
              lastButton_bottom = currentButton_bottom; //reset button value
            }

            if (choice == 2)
              player_turn = false;

            while (player_turn)
            {
              split2.add_rear(shoe.current_rank(), shoe.current_suit());
              shoe.remove_front();
              shoe.start();
              split2.start();
              for (byte i = 0; i < split2.size() - 1; i++)
                split2.advance();

              deal_card_up(3 + ((split2.size() - 1) * 2), 2, split2);
              calculate_total(split2, split2_score);
              if (split2_score[1] > 21)
              {
                lcd.setCursor(15, 2);
                lcd.print(F("Bust!"));
                playTone(2);
                money -= bet;
                split_hands--;
                player_turn = false;
              } else {
                display_split2_score(15, 2);

                if (split2_score[1] != 21)
                {
                  lcd.setCursor(16, 0);
                  lcd.print(F("Hit"));
                  lcd.print(char(5));
                }
                lcd.setCursor(15, 1);
                lcd.print(F("Stay"));
                lcd.print(char(6));

                go = false;
                byte choice;

                while (!go)
                {
                  if (split2_score[1] != 21)
                  {
                    currentButton_top = digitalRead(b_top); //read button state
                    if (lastButton_top == HIGH && currentButton_top == LOW) //if it was pressed…
                    {
                      choice = 1;
                      go = true;
                    }
                    lastButton_top = currentButton_top; //reset button value
                  }

                  currentButton_bottom = digitalRead(b_bottom); //read button state
                  if (lastButton_bottom == HIGH && currentButton_bottom == LOW) //if it was pressed…
                  {
                    choice = 2;
                    go = true;
                  }
                  lastButton_bottom = currentButton_bottom; //reset button value
                }

                if (choice == 2)
                  player_turn = false;
              }
            }

            //If both hands end up busting, the round is over and the dealer's turn gets bypassed.
            if (split1_score[1] > 21 && split2_score[1] > 21)
              dealer_turn = false;
            delay(200);
          }
        }  //End of split code

        //////////////////////////////////////////////////////////////
        // This is the choice if the player decides to double down. //
        // They only get one more card and their bet doubles.       //
        //////////////////////////////////////////////////////////////
        if (choice == 2)
        {
          double_down = true;

          //Add card to player's hand and remove card from shoe. Card is then dealt to the screen and a determination
          //is made for if the player busts or not.
          player.add_rear(shoe.current_rank(), shoe.current_suit());
          shoe.remove_front();
          shoe.start();
          player.start();
          player.advance();
          player.advance();
          deal_card_up(7, 0, player);
          calculate_total(player, player_score);
          if (player_score[1] > 21)
          {
            lcd.setCursor(7, 2);
            lcd.print(F("             "));
            lcd.setCursor(7, 3);
            lcd.print(F("             "));
            lcd.setCursor(15, 0);
            lcd.print(F("Bust!"));
            playTone(2);
            handResult = 2;       //This comes into play at the end of the hand
            money -= bet * 2;     //Double the wager, double the loss.
            dealer_turn = false;
          } else {
            display_p_score(15, 0);
            lcd.setCursor(7, 2);
            lcd.print(F("             "));
            lcd.setCursor(7, 3);
            lcd.print(F("             "));
            lcd.setCursor(16, 2);
          }
        }

        //////////////////////////////////////////////////////////////////
        // This is the choice for a standard hit after the initial deal //
        //////////////////////////////////////////////////////////////////
        if (choice == 1)
        {
          while (player_turn) //Essentially the option to hit is available until the player
          {                   //busts or stays
            player.add_rear(shoe.current_rank(), shoe.current_suit());
            shoe.remove_front();
            shoe.start();
            player.start();
            for (byte i = 0; i < player.size() - 1; i++)
              player.advance();

            deal_card_up(3 + ((player.size() - 1) * 2), 0, player);
            calculate_total(player, player_score);
            if (player_score[1] > 21)
            {
              lcd.setCursor(7, 2);
              lcd.print(F("             "));
              lcd.setCursor(7, 3);
              lcd.print(F("             "));
              lcd.setCursor(15, 0);
              lcd.print(F("Bust!"));
              handResult = 2;
              money -= bet;
              player_turn = false;
              dealer_turn = false;
            } else {
              display_p_score(15, 0);
              lcd.setCursor(7, 2);
              lcd.print(F("             "));
              lcd.setCursor(7, 3);
              lcd.print(F("             "));
              lcd.setCursor(16, 2);
              if (player_score[1] != 21)
              {
                lcd.print(F("Hit"));
                lcd.print(char(5));
              }
              lcd.setCursor(15, 3);
              lcd.print(F("Stay"));
              lcd.print(char(6));

              go = false;
              byte choice;

              while (!go)
              {
                if (player_score[1] != 21)
                {
                  currentButton_top = digitalRead(b_top); //read button state
                  if (lastButton_top == HIGH && currentButton_top == LOW) //if it was pressed…
                  {
                    choice = 1;
                    go = true;
                  }
                  lastButton_top = currentButton_top; //reset button value
                }

                currentButton_bottom = digitalRead(b_bottom); //read button state
                if (lastButton_bottom == HIGH && currentButton_bottom == LOW) //if it was pressed…
                {
                  choice = 2;
                  go = true;
                }
                lastButton_bottom = currentButton_bottom; //reset button value
              }

              if (choice == 2)
                player_turn = false;
            }
          }
        }

        /////////////////////////////////////////////////////////////////////////////
        // Once all of the player's cards have been dealt, it is the dealer's turn //
        /////////////////////////////////////////////////////////////////////////////
        if (dealer_turn)
        {
          //The animation to move the dealer's face up card back after split.
          if (split)
          {
            dealer.start();
            lcd.setCursor(0, 2);
            lcd.print(F("  "));
            lcd.setCursor(0, 3);
            lcd.print(F("  "));
            deal_card_up(1, 2, dealer);
            delay(200);
            lcd.setCursor(1, 2);
            lcd.print(F("  "));
            lcd.setCursor(1, 3);
            lcd.print(F("  "));
            deal_card_up(2, 2, dealer);
            delay(200);
            lcd.setCursor(2, 2);
            lcd.print(F("  "));
            lcd.setCursor(2, 3);
            lcd.print(F("  "));
          }

          //This block seems redundant, but is done to clear out second split row if player splits
          lcd.setCursor(7, 2);
          lcd.print(F("             "));
          lcd.setCursor(7, 3);
          lcd.print(F("             "));
          lcd.setCursor(0, 2);
          lcd.print(F("D: "));
          dealer.start();
          deal_card_up(3, 2, dealer);
          dealer.advance();
          deal_card_down(5, 2);
          delay(1000);

          //This block turns the card that is facing down to facing up
          dealer.start();
          dealer.advance();
          deal_card_up(5, 2, dealer);
          calculate_total(dealer, dealer_score);
          display_d_score(15, 3);

          //Cards are dealt out to the dealer until 17 or above is reached.
          while (dealer_score[1] < 17)
          {
            delay(1000);
            dealer.add_rear(shoe.current_rank(), shoe.current_suit());
            shoe.remove_front();
            shoe.start();
            dealer.start();
            for (byte i = 0; i < dealer.size() - 1; i++)
              dealer.advance();

            deal_card_up(3 + ((dealer.size() - 1) * 2), 2, dealer);
            calculate_total(dealer, dealer_score);
            display_d_score(15, 3);
          }
          delay(1000);

          //If the dealer busts the player is paid and the turn ends
          if (dealer_score[1] > 21)
          {
            lcd.setCursor(8, 3);
            lcd.print(F("Dealer busts"));
            handResult = 1;
            if (double_down)
              money += bet * 2;
            else if (split)
              money += bet * split_hands;
            else
              money += bet;
          } else if (split)   //If the dealer doesn't bust and the player's hand was split,
          {                   //then each split is evaluated to see if player or dealer wins.
            if (split1_score[1] <= 21)
            {
              lcd.setCursor(0, 0);
              lcd.print(F("1:"));
              lcd.setCursor(15, 0);
              lcd.print(F("     "));
              lcd.setCursor(15, 1);
              lcd.print(F("     "));
              display_split1_score(15, 0);
              if (split1_score[1] > dealer_score[1])
              {
                lcd.setCursor(12, 1);
                lcd.print(F("You won!"));
                playTone(1);
                money += bet;
              } else if (split1_score[1] < dealer_score[1]) {
                lcd.setCursor(14, 1);
                lcd.print(F("Dealer"));
                lcd.setCursor(16, 2);
                lcd.print(F("wins"));
                money -= bet;
                playTone(2);
              } else {
                lcd.setCursor(16, 2);
                lcd.print(F("Push"));
                playTone(3);
              }
              delay(2000);
            }

            if (split2_score[1] <= 21)
            {
              lcd.setCursor(0, 0);
              lcd.print(F("2:             "));
              lcd.setCursor(0, 1);
              lcd.print(F("                    "));
              lcd.setCursor(15, 0);
              lcd.print(F("     "));
              lcd.setCursor(15, 1);
              lcd.print(F("     "));
              lcd.setCursor(15, 2);
              lcd.print(F("     "));
              byte loc = 3;
              for (split2.start(); split2.is_item(); split2.advance())
              {
                deal_card_up(loc, 0, split2);
                loc += 2;
              }
              display_split2_score(15, 0);
              if (split2_score[1] > dealer_score[1])
              {
                lcd.setCursor(12, 1);
                lcd.print(F("You won!"));
                playTone(1);
                money += bet;
              } else if (split2_score[1] < dealer_score[1]) {
                lcd.setCursor(14, 1);
                lcd.print(F("Dealer"));
                lcd.setCursor(16, 2);
                lcd.print(F("wins"));
                money -= bet;
                playTone(2);
              } else {
                lcd.setCursor(16, 2);
                lcd.print(F("Push"));
                playTone(3);
              }
            }

            handResult = 0; //This prevents results to show in the endHand function since they are meant for
                            //non-split hands.

          //If hand isn't split and dealer doesn't bust, player and dealer hands are compared to see who wins
          } else if (player_score[1] > dealer_score[1])
          {
            handResult = 1;
            if (double_down)
              money += bet * 2;
            else
              money += bet;
          }
          else if (player_score[1] < dealer_score[1])
          {
            handResult = 2;
            if (double_down)
              money -= bet * 2;
            else
              money -= bet;
          }
          else
            handResult = 3;
        }

      }
      endHand();  //Runs the endHand function to display results of hand
    }

  }

  //Thy game is over. You have run out of money. This ends the setup function and halts the program until reset.
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print(F("Game  Over"));
  lcd.setCursor(2, 2);
  lcd.print(F("You have run out"));
  lcd.setCursor(6, 3);
  lcd.print(F("of money"));
}

void loop()
{
//The loop function is not used in this program.
}

///////////////////////////////////////////////////////
// Here on out are the functions used in the program //
///////////////////////////////////////////////////////
void deal_card_up(byte col, byte row, deck & deck)
{
  lcd.setCursor(col, row);
  lcd.print(face[deck.current_rank()]);
  lcd.print(char(32));
  lcd.setCursor(col, row + 1);
  lcd.print(char(32));
  lcd.print(deck.current_suit());
}

void deal_card_down(byte col, byte row)
{
  lcd.setCursor(col, row);
  lcd.print(char(255));
  lcd.print(char(255));
  lcd.setCursor(col, row + 2);
  lcd.print(char(255));
  lcd.print(char(255));
}

void initial_deal()
//This function creates the initial hands for the player and dealer from the shoe
//and deals them out.
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("P:"));
  lcd.setCursor(0, 2);
  lcd.print(F("D:"));

  dealer.start();

  player.add_rear(shoe.current_rank(), shoe.current_suit());
  shoe.remove_front();
  shoe.start();
  player.start();
  deal_card_up(3, 0, player);
  calculate_total(player, player_score);
  display_p_score(15, 0);
  delay(500);

  dealer.add_rear(shoe.current_rank(), shoe.current_suit());
  shoe.remove_front();
  shoe.start();
  dealer.start();
  deal_card_up(3, 2, dealer);
  delay(500);

  player.add_rear(shoe.current_rank(), shoe.current_suit()); //shoe.current_rank()
  shoe.remove_front();
  shoe.start();
  player.start();
  player.advance();
  deal_card_up(5, 0, player);
  calculate_total(player, player_score);
  display_p_score(15, 0);
  delay(500);

  dealer.add_rear(shoe.current_rank(), shoe.current_suit());
  shoe.remove_front();
  shoe.start();
  deal_card_down(5, 2);

  calculate_total(dealer, dealer_score);
}

void place_bet(unsigned long &bet, unsigned long &money)
//This function runs the betting screen
{
  lcd.clear();
  lcd.setCursor(2, 1);
  lcd.print(F("You have: $"));
  lcd.print(money);
  lcd.setCursor(2, 2);
  lcd.print(F("Bet: "));
  lcd.print(char(127));
  lcd.print(F(" "));
  lcd.print(char(126));
  lcd.print(F("  $"));
  lcd.setCursor(0, 3);
  bool done = 0;

  if (bet > money)
    bet = money;

  while (!done)
  {
    currentButton_left = digitalRead(b_left); 
    if (lastButton_left == HIGH && currentButton_left == LOW) 
    {
      if (bet > 10) //minimum bet is 10, not 0
      {
        bet -= 10;
      }

    }
    lastButton_left = currentButton_left; 

    currentButton_right = digitalRead(b_right); 
    if (lastButton_right == HIGH && currentButton_right == LOW) 
    {
      if (bet < money) //Maximum bet is the total money the player has
      {
        bet += 10;
      }
    }
    lastButton_right = currentButton_right; //reset button value


    currentButton_bottom = digitalRead(b_bottom); //read button state
    if (lastButton_bottom == HIGH && currentButton_bottom == LOW)
    {
      done = 1;   //The proverbial enter key

    }
    lastButton_bottom = currentButton_bottom; //reset button value

    lcd.setCursor(13, 2);
    lcd.print(bet);

    //This deletes extra spaces when the bet is reduced and the last digit is removed
    if (bet < 10000)
    {
      lcd.print(F(" "));
    }
    else if (bet < 1000)
    {
      lcd.print(F("  "));
    }
    else if (bet < 100)
    {
      lcd.print(F("   "));
    }
    lcd.setCursor(19, 3);
    lcd.print(char(6));
  }

}

void calculate_total(deck & c, byte total[2])
//This function calculates the current total of any deck object
//and split the total into a soft and hard total (if necessary)
{
  bool ace = false;
  byte tempTotal = 0;
  byte tempHigh;
  c.start();
  for (c.start(); c.is_item(); c.advance())
  {
    if (c.current_rank() == 1)
    {
      ace = true;
      tempTotal += 1;
    }
    else if (c.current_rank() >= 11)
    {
      tempTotal += 10;
    }
    else
    {
      tempTotal += c.current_rank();
    }
  }

  tempHigh = tempTotal + 10;
  if (!ace)
  {
    total[0] = 0;
    total[1] = tempTotal;
  }
  else if (tempHigh > 21)
  {
    total[0] = 0;
    total[1] = tempTotal;
  }
  else
  {
    total[0] = tempTotal;
    total[1] = tempHigh;

  }
}

void check_blackjack()
//This function checks if either (or both) the player and dealer have blackjack.  I checks against the
//condition of if the player had wagered insurance against a dealer's ace.
{
  if (has_insurance)
  {
    if (dealer_score[1] == 21)
    {
      if (player_score[1] == 21)
      {
        money += bet;
        handResult = 3;
        in_round = false;
        delay(500);
        dealer.start();
        dealer.advance();
        deal_card_up(5, 2, dealer);
        lcd.setCursor(10, 0);
        lcd.print(F("Blackjack!"));
        lcd.setCursor(10, 3);
        lcd.print(F("Blackjack!"));
        delay(500);
      }
      else
      {
        handResult = 2;
        in_round = false;
        delay(500);
        dealer.start();
        dealer.advance();
        deal_card_up(5, 2, dealer);
        lcd.setCursor(10, 3);
        lcd.print(F("Blackjack!"));
        delay(500);
      }
    }
    else
    {
      if (player_score[1] == 21)
      {
        money += bet;
        handResult = 1;
        in_round = false;
        delay(500);
        dealer.start();
        dealer.advance();
        deal_card_up(5, 2, dealer);
        lcd.setCursor(10, 0);
        lcd.print(F("Blackjack!"));
        delay(500);
      }
      else
      {
        money -= bet / 2;
      }
    }
  }
  else
  {
    if (dealer_score[1] != 21)
    {
      if (player_score[1] == 21)
      {
        money += bet * 1.5;
        handResult = 1;
        in_round = false;
        delay(500);
        dealer.start();
        dealer.advance();
        deal_card_up(5, 2, dealer);
        lcd.setCursor(10, 0);
        lcd.print(F("Blackjack!"));
        delay(500);
      }
    }
    else
    {
      if (player_score[1] != 21)
      {
        money -= bet;
        handResult = 2;
        in_round = false;
        delay(500);
        dealer.start();
        dealer.advance();
        deal_card_up(5, 2, dealer);
        lcd.setCursor(10, 3);
        lcd.print(F("Blackjack!"));
        delay(500);
      }
      else
      {
        handResult = 3;
        in_round = false;
        delay(500);
        dealer.start();
        dealer.advance();
        deal_card_up(5, 2, dealer);
        lcd.setCursor(10, 0);
        lcd.print(F("Blackjack!"));
        lcd.setCursor(10, 3);
        lcd.print(F("Blackjack!"));
        delay(500);

      }
    }
  }
}

void insurance()
//This is the function that checks if the dealer is holding an ace and asks if the
//player wishes to purchase insurance (at the price of 1/2 the bet).
{
  dealer.start();
  if (dealer.current_rank() == 1)
  {
    lcd.setCursor(10, 2);
    lcd.print(F("Insurance?"));
    lcd.setCursor(10, 3);
    lcd.print(char(127));
    lcd.print(F("Yes "));
    lcd.print(char(126));
    lcd.print(F("No  "));

    bool done = false;

    while (!done)
    {
      currentButton_left = digitalRead(b_left); //read button state
      if (lastButton_left == HIGH && currentButton_left == LOW) //if it was pressed…
      {
        has_insurance = true;
        done = 1;
      }
      lastButton_left = currentButton_left; //reset button value

      currentButton_right = digitalRead(b_right); //read button state
      if (lastButton_right == HIGH && currentButton_right == LOW) //if it was pressed…
      {
        has_insurance = false;
        done = 1;
      }
      lastButton_right = currentButton_right; //reset button value
    }
    lcd.setCursor(10, 2);
    lcd.print(F("          "));
    lcd.setCursor(10, 3);
    lcd.print(F("          "));
  }
}

void endHand()
//This function is called at the end of a hand. Depending on the hardResult flag,
//the result of the hand is displayed on the screen
{
  player.clear();
  dealer.clear();

  switch (handResult)
  {
    case 1:
      lcd.setCursor(12, 1);
      if (!split)
        lcd.print(F("You won!"));
      playTone(1);
      break;
    case 2:
      lcd.setCursor(14, 1);
      lcd.print(F("Dealer"));
      lcd.setCursor(16, 2);
      lcd.print(F("wins"));
      playTone(2);
      break;
    case 3:
      lcd.setCursor(16, 2);
      lcd.print(F("Push"));
      playTone(3);
      break;
  }

  //Some housecleaning to make sure flags are reset for the next hand
  in_round = true;
  player_turn = true;
  dealer_turn = true;
  double_down = false;
  has_insurance = false;
  split_hands = 0;
  split = false;
  split1.clear();
  split2.clear();
  delay(3000);
}

bool check_split()
//A simple function to check if the split menu item should display
{
  player.start();
  byte rank1 = player.current_rank();
  player.advance();
  byte rank2 = player.current_rank();
  if (rank1 == rank2)
    return true;
  else
    return false;
}

//The next four functions display the scores of the player, dealer, split 1, and split 2 respectively
void display_p_score(byte col, byte row)
{
  lcd.setCursor(col, row);
  lcd.print(F("   "));
  lcd.setCursor(col + 3, row);
  lcd.print(player_score[1]);
  if (player_score[0] != 0)
  {
    lcd.setCursor(col, row);
    lcd.print(player_score[0]);
    lcd.print(F("/"));
  }
}

void display_d_score(byte col, byte row)
{
  lcd.setCursor(col + 3, row);
  lcd.print(dealer_score[1]);
  if (dealer_score[0] != 0)
  {
    lcd.setCursor(col, row);
    lcd.print(dealer_score[0]);
    lcd.print(F("/"));
  }
}

void display_split1_score(byte col, byte row)
{
  lcd.setCursor(col, row);
  lcd.print(F("   "));
  lcd.setCursor(col + 3, row);
  lcd.print(split1_score[1]);
  if (split1_score[0] != 0)
  {
    lcd.setCursor(col, row);
    lcd.print(split1_score[0]);
    lcd.print(F("/"));
  }
}

void display_split2_score(byte col, byte row)
{
  lcd.setCursor(col, row);
  lcd.print(F("   "));
  lcd.setCursor(col + 3, row);
  lcd.print(split2_score[1]);
  if (split2_score[0] != 0)
  {
    lcd.setCursor(col, row);
    lcd.print(split2_score[0]);
    lcd.print(F("/"));
  }
}

void playTone(byte t)
//This function simply plays the tones for win, loss, or push
{
  switch (t)
  {
    case 1:
      tone(SPEAKER, 1046, 100);
      delay(100);
      tone(SPEAKER, 1318, 100);
      delay(100);
      tone(SPEAKER, 1567, 100);
      delay(100);
      break;
    case 2:
      tone(SPEAKER, 110, 1000);
      delay(1000);
      break;
    case 3:
      tone(SPEAKER, 300, 200);
      delay(400);
      tone(SPEAKER, 300, 800);
      delay(800);
      break;
  }
}

