#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

typedef struct OUT{
	char *Date;
	char *Domain;
}OUT;

	const unsigned char month_epoch = 1, day_epoch = 1, hour_poch = 0, minute_poch = 0;
	const int year_epoch = 1970;
	const double sec_ms_epoch = 0;
	unsigned char leap_years =0;
	const unsigned long month_s[] = {2678400, 5097600, 7776000,10368000,13046400,15638400,18316800,20995200,23587200,26265600,28857600,31536000};

uint64_t convert_date_to_epoch(char* line){ // requires trimmed version
	uint64_t  ret_res = 0;
	char* year = (char*)calloc(5, sizeof(char)); // format 2017 +1 for 0x00
	char* month = (char*)calloc(10, sizeof(char)); // format 12 +1 for 0x00
	char* day = (char*)calloc(3, sizeof(char)); // format 27 +1 for 0x00
	char* hour= (char*)calloc(3, sizeof(char));
	char* minute= (char*)calloc(3, sizeof(char));
	char* sec_ms= (char*)calloc(13, sizeof(char));
	char* sec = (char*)calloc(3, sizeof(char));
	char* ms = (char*)calloc(4, sizeof(char));
	/*
		January
		February
		March
		April
		May
		June
		July
		August
		September
		October
		November
		December
		J = 3, A = 2, M = 2, N = 1, O = 1, F = 1, S = 1, D = 1;
	*/
	unsigned char month_i, day_i, hour_i, minute_i;
	int year_i = 0;
	double sec_ms_i = 0;
	unsigned char counter = 0;
	char *t_offset = line;
	// 1509006715
	// find first space
	// 18-May-2018 22:00:17.049 queries: info: client 192.168.1.200#59241 (d.dropbox.com): query: d.dropbox.com IN A + (192.168.1.254
	while(*line != '-'){ counter++; line++; } // get the day
	memcpy(day, t_offset, counter); counter = 0;
	t_offset = ++line; // after should point to Month
	while(*line != '-'){ counter++; line++; }
	memcpy(month, t_offset, counter); counter = 0;
	t_offset = ++line; // pointing at year now
	while(*line != ' '){ counter++; line++; }
	memcpy(year, t_offset, counter); counter =0;
	t_offset = ++line; // pointing at time
	memcpy(hour, line, 2); // copy time [22]
	line += 3;
	memcpy(minute, line, 2); // get minute
	line += 3;
	memcpy(sec, line, 2); // seconds
	line += 3;
	memcpy(ms, line, 3); // ms
	// if bombing for months
	if (month[0] == 'D'){month[0] = '1'; month[1] = '2'; month[2] = 0; }
	if (month[0] == 'S'){month[0] = '9'; month[1] = 0; }
	if (month[0] == 'F'){month[0] = '2'; month[1] = 0; }
	if (month[0] == 'O'){month[0] = '1'; month[1] = 0; month[2] = 0; }
	if (month[0] == 'N'){month[0] = '1'; month[1] = '1'; month[2] = 0; }
	if (month[0] == 'M'){
		switch(month[2]){
			case 'r':
				month[0] = '3'; month[1] = 0;
				break;
			case 'y':
				month[0] = '5'; month[1] = 0;
				break;
		}
	}
	if (month[0] == 'A'){
		switch(month[2]){
                        case 'p':
                                month[0] = '4'; month[1] = 0;
                                break;
                        case 'u':
                                month[0] = '8'; month[1] = 0;
                                break;
                }
	}
	if (month[0] == 'J'){
		switch(month[3]){
                        case 'u':
                                month[0] = '1'; month[1] = 0;
                                break;
                        case 'e':
                                month[0] = '6'; month[1] = 0;
                                break;
			case 'y':
				month[0] = '7'; month[1] = 0;
				break;
                }
	}
	// end of bombing

	// conversion
	year_i = atoi(year)		-year_epoch;	// this may differ, compensate for 0th year
	month_i = atoi(month)	-month_epoch;	// this may differ
	day_i = atoi(day)		-day_epoch;		// this may differ
	hour_i = atoi(hour);				// time offset is always 0, no need to subtract
	minute_i = atoi(minute);
//	sec_ms_i = atof(sec_ms)*1000000.0;	// ns precision dd.mmmnnn
	sec_ms_i = atof(sec)*1000000.0;
	sec_ms_i += atof(ms)*1000.0;
	ret_res += (365*year_i)*86400;
	ret_res += month_s[month_i-1];//2628000; 
	ret_res += (day_i)*86400;
	ret_res += hour_i*3600;
	ret_res += minute_i*60;

	if (leap_years == 0) leap_years= ++year_i/4;
	ret_res += (leap_years)*86400;
	ret_res *= 1000000;
	ret_res += (int)sec_ms_i;

	//capture year until - is found
	//capture month until - is found
	//capture day until ' ' is found

	free(year);
	free(month);
	free(day);
	free(hour);
	free(minute);
	free(sec_ms);

	return ret_res;
}


struct OUT* process_line(const char* line){
	struct OUT *output = (struct OUT*)calloc(1, sizeof(struct OUT));
	// extract date
	char *ptr = (char*)line;
	unsigned char space_count = 0;
	unsigned int position;
	while(ptr++){
		if (*ptr == ' ') space_count++;
		if (space_count == 2){ space_count = 0; position++; break; } 
		position++;
	}
	output->Date = (char*) calloc(position+1, sizeof(char*));
	memcpy(output->Date, line, position);
	position = 0;
	while(ptr++){
		if (*ptr == 0)    return (struct OUT*)NULL;
		if (*ptr == '\n') return (struct OUT*)NULL;
		if (*ptr == '\r') return (struct OUT*)NULL;
		if ((*ptr == 'q') && (*(ptr+1) == 'u') &&(*(ptr+2) == 'e') &&(*(ptr+3) == 'r') &&(*(ptr+4) == 'y') && (*(ptr+5) == ':')){
			 // got to the query
			ptr += 7; // get : and ' '
			break;
		}
	}
	// extract domain
	position = 0;
	char *domain = ptr;
	while(ptr++){
                if (*ptr == ' ') space_count++;
                if (space_count == 1){ space_count = 0; position++; break; }
                position++;
        }
	output->Domain= (char*) calloc(position+1, sizeof(char*));
	memcpy(output->Domain, domain, position);
	return output;
}

int main(){
	char line_buff[2048];
	while(fgets(line_buff, sizeof(line_buff), stdin) != 0){
		struct OUT* out = process_line(line_buff);
		uint64_t date = convert_date_to_epoch(out->Date);
		printf("%" PRIu64 ",%s\n", date, out->Domain);
	}
	return 0;
}
