/*
   This class creates an object that is a doubly ended queue of objects, in this case, playing cards.
*/
#include "card.h"
#include <assert.h>
#include <LiquidCrystal.h>

class deck
{
  public:

    deck()  //Constructor
    {
      front_ptr = rear_ptr = NULL;
      count = 0;
      num_decks = 0;
    }

    ~deck() //Deconstructor
    {
      clear();
    }

    void add_front(byte rank, char suit)
    //Adds a card to the front of the deck
    //Required parameters are the rank and suit of the new card
    {
      if (front_ptr == NULL) //Deck is empty
      {
        front_ptr = new card(rank, suit, NULL, NULL);
        rear_ptr = front_ptr;
      }
      else
      {
        assert(!empty());
        card *temp_ptr = new card(rank, suit, NULL, front_ptr);
        front_ptr->set_link_left(temp_ptr);
        front_ptr = temp_ptr;
      }
      count++;
    }

    void add_rear(byte rank, char suit)
    //Adds a card to the rear of the deck
    //Required parameters are the rank and suit of the new card
    {
      if (front_ptr == NULL)
      {
        front_ptr = new card(rank, suit, NULL, NULL);
        rear_ptr = front_ptr;
      }
      else
      {
        assert(!empty());
        card *temp_ptr = new card(rank, suit, rear_ptr, NULL);
        rear_ptr->set_link_right(temp_ptr);
        rear_ptr = temp_ptr;
      }
      count++;
    }

    void remove_front()
    //Removes the first card of the deck
    {
      assert(!empty());
      card* temp_ptr = front_ptr;
      front_ptr = front_ptr->get_link_right();
      delete temp_ptr;
      count--;

      if (front_ptr == NULL)
        rear_ptr = NULL;

      if (front_ptr != NULL)
        front_ptr->set_link_left(NULL);
    }

    void remove_rear()
    //Removes the last card of the deck
    {
      assert(!empty());
      card* temp_ptr = rear_ptr;
      rear_ptr = rear_ptr->get_link_left();
      delete temp_ptr;
      count--;

      if (rear_ptr == NULL)
        front_ptr = NULL;

      if (rear_ptr != NULL)
        rear_ptr->set_link_right(NULL);
    }

    bool empty() {
      return (count == 0);  //Returns true if deck of cards is empty, false if it's not
    }

    bool find(byte rank, char suit, byte num, byte location)
    //Search through deck and return true if card is found, false if it is not
    //This method is primarily used in the shuffle method
    //rank = rank of card you are searching for
    //suit = suit of card you are searching for
    //num = number of decks
    //location = starting location (or deck number) for the search (due to possibility of mulitple decks)
    //Because the shuffling of the deck is collated (ex. {deck1/card1, deck2/card1, deck3/card1, deck1/card2,
    //deck2/card2, deck3/card2,...}) this method only searches through the particular deck you specify in
    //the location parameter.
    {
      card *index = front_ptr;

      for (int i = 0; i < location; i++)
        index = index->get_link_right();

      while (index != NULL)
      {

        if (index->data1() == rank && index->data2() == suit)
          return true;

        for (int i = 0; i < num; i++)
          index = index->get_link_right();
      }
      return false;
    }

    void start() 
    //Places cursor at the first card in the deck
    {
      cursor = front_ptr;
    }

    void advance() 
    //Advances cursor to the next card in the deck
    {
      if (is_item())
        cursor = cursor->get_link_right();
    }

    void clear()
    //Deletes all the cards in the deck
    {
      if (!empty())
      {
        while (count != 0)
          remove_front();
      }
    }

    void shuffle(LiquidCrystal &lcd)
    //The name of this method is somewhat deceiving since it doesn't actually shuffle an existing
    //deck of cards.  It empties out the deck and creates a new deck of 52 randomly shuffle cards
    //if there is one deck, and a collated deck of cards if there is more than one deck.
    //(ex. {deck1/card1, deck2/card1, deck3/card1, deck1/card2, deck2/card2, deck3/card2,...}).
    //For the purpose of blackjack, the cards are shuffled only when the shoe is running short of
    //cards (approx. 75% of total).  The LCD parameter is passed to allow this method to print onto
    //the LCD object created in the main program.
    {
      clear();
      byte dots = 0;
      lcd.clear();
      lcd.setCursor(3, 1);
      lcd.print(F("Shuffling deck"));
      randomSeed(analogRead(A0));   //Generate the random seed based on the floating analog pin A0.
      while (count < 52 * num_decks)
      {
        byte location = count % num_decks;  //Creates the start location for the 'find' method
        byte rank = random(1, 14);         //Create random rank
        char suit = random(1, 5);          //Create random suit
        
        //If the randomly created card is not found in the exisitng deck, add it to the rear.
        //This continues until all the cards are filled.
        if (!find(rank, suit, num_decks, location))
        {
          add_rear(rank, suit);
          dots = count / num_decks / 2.6;  //Generates a number of dots on the screen based on the percentage
        }                                  //of cards filled in the deck to the expected deck total. (2.6 is
        lcd.setCursor(0, 2);               //derived by number of cards in a deck divided by number of columns
        for (int i = 0; i < dots; i++)     //on the LCD screen
          lcd.print(F("."));
      }
      start();  //Puts the cursor on the first card in preperation for dealing.
    }


    byte current_rank() const {
      return cursor->data1();  //Return rank of card the cursor is pointing to
    }
    
    char current_suit() const {
      return cursor->data2();  //Return suit of card the cursor is pointing to
    }
    
    bool is_item() const {
      return cursor != NULL;  //Returns true or false if cursor is on a card
    }

    byte size() const {
      return count;           //Returns the current number of cards in the deck
    }

    byte numDecks() const {
      return num_decks;       //Returns the number of decks
    }

    void setDecks(byte num) {
      num_decks = num;        //Sets the number of decks
    }


  private:

    byte count;
    byte num_decks;
    card *front_ptr, *rear_ptr, *cursor;
};
