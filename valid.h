#include <stdio.h>
#include <string.h>

#ifndef VALID_H
#define VALID_H

#define USER_MIN_LEN		5
#define USER_MAX_LEN		30
#define PASS_MIN_LEN		5
#define PASS_MAX_LEN		30
#define EMAIL_MIN_LEN		6
#define EMAIL_MAX_LEN		50
#define SUSC_NAME_MIN_LEN	5
#define SUSC_NAME_MAX_LEN	30
#define SITE_NAME_MIN_LEN	3
#define SITE_NAME_MAX_LEN	30
#define DOMAIN_MIN_LEN		2
#define DOMAIN_MAX_LEN		50
#define DB_NAME_MIN_LEN		3
#define DB_NAME_MAX_LEN		30
#define SIZE_MIN_LEN		1	//Permite representar 1byte
#define SIZE_MAX_LEN		12	//Permite representar casi 1TB
/* Las siguientes dos definiciones aplica
 * para todas las estructuras internas que
 * tienen id. */
#define ID_MIN_LEN		1
#define ID_MAX_LEN		20

int valid_user_name(char *s);
int valid_passwd(char *s);
int valid_email(char *s);
int valid_domain(char *s);
int valid_namespace_name(char *s);
int valid_id(char *s);
int valid_size(char *s);
int valid_ftp_name(char *s);
int valid_site_name(char *s);

#endif
