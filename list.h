#ifndef __LIST_H_
#define __LIST_H_

#include <stdbool.h>

/*
 * A ist item which can store any given object. The list is a two directional
 * list meaning the previous position and the next position can be found of a
 * given position. The objects given to the list will not be frees, so the user
 * of this list should handle that.
 *
 * Authors: Bram Coenen (brco0009@student.umu.se)
 *
 * Version information:
 *   2018-09-13: v1.0.
 */

// The list type.
typedef struct list{
	struct node *head;
	struct node *end;
}list;

// The list position type.
typedef struct node *list_pos;

/**
 * list_new() - Create a new and empty list.
 * Returns: A pointer to the new list.
 */
list* list_new(void);

/**
 * list_add() - Adds a new node in front of the given position.
 * @val: A value to add to the newly created node.
 * @pos: A position which the new value should be stored after.
 * @l: A list which to add the value to.
 * Returns: A pointer to the new position.
 * Or NULL if trying to add after end.
 */
list_pos list_add(void* val, list_pos pos, list* l);

/**
 * list_append() - Adds a new node at the end of the list. Calls list_add();
 * @val: A value to add to the newly created node.
 * @l: A list which to add the value to.
 * Returns: A pointer to the new position.
 */
list_pos list_append(void *val, list* l);

/**
 * list_swap() - Swaps two nodes in the list;
 * @pos_one: The first position.
 * @pos_two: The second position.
 */
void  list_swap(list_pos pos_one, list_pos pos_two);

/**
 * list_is_empty() - Checks if the given list is empty.
 * @l: A list the check for emptiness.
 * Returns: true if the list is empty, else false.
 */
bool list_is_empty(list* l);

/**
 * list_get_value() - Gets the value of the element at the given position.
 * @pos: The position which to get the element from.
 * Returns: The element connected to the position.
 * Or NULL if header or end position.
 * Returns: The item connected to the postion.
 */
void* list_get_value(list_pos pos);

/**
 * list_get_first_position() - Gets the first position of the list.
 * @l: The list which to get the first position from.
 * Returns: The position of the header position of the list. Which is
 * not an added element.
 * Returns: Pointer to the header position
 */
list_pos list_get_first_position(list *l);

/**
 * list_get_last_position() - Gets the last position of the list.
 * @l: The list which to get the last position from.
 * Returns: The end position of the list. Which is
 * not an added element.
 * Returns: Pointer to the end position.
 */
list_pos list_get_last_position(list *l);

/**
 * list_get_next_position() - Gets the next position in the list.
 * @pos: The postion where to get the next position of.
 * @l: The list which to get the next position from.
 * Returns: The position of the element further from the
 * first position and closer to the end postion.
 * Or NULL if the given position is the header position or
 * the end position.
 * Returns: Pointer to the next position
 */
list_pos list_get_next_position(list_pos pos, list *l);

/**
 * list_get_previous_position() - Gets the pervious position in the list.
 * @pos: The postion where to get the next position of.
 * @l: The list which to get the previous position from.
 * Returns: The position of the element further from the
 * end position and closer to the header postion.
 * Or NULL if the given position is the header position or
 * the end position.
 * Returns: Pointer to the previous position.
 */
list_pos list_get_previous_position(list_pos pos, list *l);

/**
 * list_remove_element() - Removes the element from the list.
 * The removed element is not freed.
 * @pos: The postion the element which to remove.
 * @l: The list which toremove the element from.
 * Returns: The position before the removed element.
 */
list_pos list_remove_element(list_pos pos, list *l);

/**
 * list_kill() - Removes the list but the elements are not freed.
 * @l: The list which to remove.
 */
void list_kill(list *l);


#endif //__LIST_H_
