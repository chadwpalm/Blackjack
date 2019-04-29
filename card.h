/*
 * This class creates a card object. It contains the rank (data1_field), suit(data2_field),
 * Pointer that links to the card to it's left, and a pointer that links to the card on it's right.
 */
#include <Arduino.h>

class card
{

  public:

    byte& data1() {           //Returns the rank
      return data1_field;
    }

    char& data2() {           //Returns the suit
      return data2_field;
    }

    card* get_link_right() {  //Returns the right-link pointer
      return link_right;
    }

    card* get_link_left() {   //Returns the left-link pointer
      return link_left;
    }

    void set_data1(const byte& new_data1) {   //Sets the rank
      data1_field = new_data1;
    }

    void set_data2(const char& new_data2) {   //Sets the suit
      data1_field = new_data2;
    }

    void set_link_right(card* new_link) {     //Sets the right-link pointer
      link_right = new_link;
    }

    void set_link_left(card* new_link) {      //Sets the left-link pointer
      link_left = new_link;
    }

    card(const byte& init_data1, const char& init_data2, card* init_link_left, card* init_link_right)
    //Constructor. Parameters required are {rank, suit, left pointer, right pointer}
    {
      data1_field = init_data1;
      data2_field = init_data2;
      link_right = init_link_right;
      link_left = init_link_left;
    }

  private:
    byte data1_field;   //Rank variable
    char data2_field;   //Suit variable
    card *link_left, *link_right;  //Left-link and right-link pointers

};
