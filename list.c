#include "list.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

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

// The list node type.
struct node{
	struct node *next;
	struct node *prev;
	void *val;
};

/**
 * list_new() - Create a new and empty list.
 * Returns: A pointer to the new list.
 */
list* list_new(void){

	list *l = calloc(1, sizeof(*l));
	if(l == NULL){
		perror("list.c");
		exit(errno);
	}

	l->head = calloc(1, sizeof(struct node));
	if(l->head == NULL){
		perror("list.c");
		exit(errno);
	}

	l->end = calloc(1, sizeof(struct node));
	if(l->end == NULL){
		perror("list.c");
		exit(errno);
	}


	l->head->next = l->end;
	l->head->prev = NULL; //Is header.
	l->head->val = NULL;

	l->end->next = NULL; //Is end
	l->end->prev = l->head;
	l->end->val = NULL; //Is end

	return l;
}

/**
 * list_add() - Adds a new node in front of the given position.
 * @val: A value to add to the newly created node.
 * @pos: A position which the new value should be stored after.
 * @l: A list which to add the value to.
 * Returns: A pointer to the new position.
 * Or NULL if trying to add after end.
 */
list_pos list_add(void *val, list_pos pos, list* l){

	if(pos == l->end){
		return NULL;
	}

	//Get pointer the current positions.
	list_pos before_pos = pos;
	list_pos after_pos = pos->next;

	list_pos new_pos=calloc(1, sizeof(struct node));
	if(new_pos == NULL){
		perror("list.c");
		exit(errno);
	}

	//Pointer for the new position
	new_pos->next = after_pos;
	new_pos->prev = before_pos;
	new_pos->val = val;

	//Fix before position
	before_pos->next = new_pos;

	//Fix after position
	after_pos->prev = new_pos;

	return new_pos;

}

/**
 * list_append() - Adds a new node at the end of the list. Calls list_add();
 * @val: A value to add to the newly created node.
 * @l: A list which to add the value to.
 * Returns: A pointer to the new position.
 */
list_pos list_append(void *val, list* l){
	return list_add(val, l->end->prev, l);
}

/**
 * list_swap() - Swaps two nodes in the list;
 * @pos_one: The first position.
 * @pos_two: The second position.
 */
void list_swap(list_pos pos_one, list_pos pos_two){
	void *temp = pos_one->val;
	pos_one->val = pos_two->val;
	pos_two->val = temp;
}

/**
 * list_is_empty() - Checks if the given list is empty.
 * @l: A list the check for emptiness.
 * Returns: true if the list is empty, else false.
 */
bool list_is_empty(list* l){
	if(l->head->next == l->end){
		return true;
	}
	else {
		return false;
	}
}

/**
 * list_get_value() - Gets the value of the element at the given position.
 * @pos: The position which to get the element from.
 * Returns: The element connected to the position.
 * Or NULL if header or end position.
 * Returns: The item connected to the postion.
 */
void* list_get_value(list_pos pos){
	return pos->val;
}

/**
 * list_get_first_position() - Gets the first position of the list.
 * @l: The list which to get the first position from.
 * Returns: The position of the header position of the list. Which is
 * not an added element.
 * Returns: Pointer to the header position
 */
list_pos list_get_first_position(list *l){
	return l->head;
}

/**
 * list_get_last_position() - Gets the last position of the list.
 * @l: The list which to get the last position from.
 * Returns: The end position of the list. Which is
 * not an added element.
 * Returns: Pointer to the end position.
 */
list_pos list_get_last_position(list *l){
	return l->end;
}

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
list_pos list_get_next_position(list_pos pos, list *l){
	if(pos == l->end){
		return NULL;
	}
	else{
		return pos->next;
	}
}

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
list_pos list_get_previous_position(list_pos pos, list *l){
	if(pos == l->head){
		return NULL;
	}
	else{
		return pos->prev;
	}
}

/**
 * list_remove_element() - Removes the element from the list.
 * The removed element is not freed.
 * @pos: The postion the element which to remove.
 * @l: The list which to remove the element from.
 * Returns: The position before the removed element.
 */
list_pos list_remove_element(list_pos pos, list *l){
	if(pos == l->head || pos == l->end){
		return l->head;
	}
	else{
		list_pos prev_pos = pos->prev;
		pos->prev->next = pos->next;
		pos->next->prev = pos->prev;

		free(pos);
		return prev_pos;
	}
}

/**
 * list_kill() - Removes the list but the elements are not freed.
 * @l: The list which to remove.
 */
void list_kill(list *l){
	list_pos current_pos = l->head->next;
	while(current_pos != l->end){
		list_pos delete_pos = current_pos;
		current_pos = current_pos->next;
		free(delete_pos);
	}

	free(l->head);
	free(l->end);
	free(l);
}
