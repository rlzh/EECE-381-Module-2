/*
 * utilities.c
 *
 *  Created on: Mar 14, 2013
 *
 */

#include "utilities.h"

Song createSong(char* name, unsigned short int* id){
	Song s;
	s.songId = *id;
	s.songName = name;
	s.songState = 0;
	s.songBuffer = NULL;
	s.songSize = 0;
	//s.header = ;
	(*id)++;
	return s;
}

Playlist createPlaylist(){
	Playlist p;
	p.numOfSongs = 0;
	p.list[MAX_SONGS_ALLOWED] = 0;
	p.order[MAX_SONGS_ALLOWED] = 0;
	p.currentSong = 0;
	return p;
}

int addToPlaylist(Song s, PlaylistPtr ptr){
	int i;
	if ((*ptr).numOfSongs == MAX_SONGS_ALLOWED) // check if playlist has reached limit
		return 2;

	for (i = 0; i < (*ptr).numOfSongs; i++){		// check if song is already in playlist
		if (s.songId == ((*ptr).list[i]))
			return 1;
	}

	(*ptr).list[(*ptr).numOfSongs] = s.songId;		// add song to end of playlist
	(*ptr).numOfSongs++;
	return 0;
}

int removeFromPlaylist(Song s, PlaylistPtr ptr){
	int i;
	int temp;
	if ((*ptr).numOfSongs == 0) // check if playlist is empty
		return 1;

	/*
	 * IMPLEMENTATION NOTE: removing song by brute force right now
	 * 						could switch to use recursion later on.
	 */
	for (i = 0; i < (*ptr).numOfSongs; i++){		// check if song is in playlist
		if (s.songId == ((*ptr).list[i])){
			temp = i;
			i = (*ptr).numOfSongs;
		}
	}
	for (i = temp; i < (*ptr).numOfSongs; i++){	// shift songs 1 over
		if (i == ((*ptr).numOfSongs - 1))
			(*ptr).list[i] = 0;
		else
			(*ptr).list[i] = (*ptr).list[i+1];
	}

	(*ptr).numOfSongs--;
	return 0;
}




