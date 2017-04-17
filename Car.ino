

//{ Setting
//Headers
#include <Wire.h>						//I2C통신을 위한 Wire
#include <utility/Adafruit_MCP23017.h>	//LCDShield
#include <Adafruit_RGBLCDShield.h>		//LCDShield
#include <SoftwareSerial.h>				//SoftwareSerial
#include <FPS_GT511C3.h>				//FingerPrints
#include <Timer.h>						//Timer
#include <NewPing.h>					//Ultrasonic sensor

//메뉴 구조체 선언
typedef struct node {
	char *title;			// 메뉴이름
	struct node *parent; 	// 부모노드
	struct node *pre;		// 이전노드
	struct node *next;		// 다음노드
	struct node *child;		// 자식노드
	char *value;			// 노드값(설정값)
} menuItem;

int fps_user[200][2]{0};

//상수값 설정
const int LCD_OFF=0x00;
const int RED=0x1;
const int YELLOW=0x3;
const int GREEN=0x2;
const int TEAL=0x6;
const int BLUE=0x4;
const int VIOLET=0x5;
const int WHITE=0x7;

const int sensingPeriod = 100;
const int cert_light = 36;

const float Crv1=1;
const float Crv2=0.8;
const float Crv3=0.7;
const float Crv4=0.3;


//LINE = 라인검출시 센서값
//NOLINE = 비라인 검출시 센서값
//0 = 흰색 , 1 = 검정색
const int LINE = 0;
const int NOLINE=1;

//포트설정
const int p_spk = 9;
const int led = 12;

const int fps_rx = 11;
const int fps_tx = 10;

const int sens_pir = 8;
const int sens_tring = 36;
const int sens_echo = 37;
const int sens_cds = 69;	// A15 = 69

const int RMT_pwm = 3;		//754410의 9번
const int RMT_f = 33;		//754410의 15번
const int RMT_b = 32;		//754410의 10번
const int LMT_pwm = 2;		//754410의 1번
const int LMT_f = 31;		//754410의 2번
const int LMT_b = 30;		//754410의 7번
const int bal = 46;
const int sens_su[] = {39,40,41,42,43,44,45};

//변수설정
int buttons=0;
int lastBTN=0;
int light=0;
int pir=0;
int distance=0;
int spk=0;
int login_user=-1;
int lcdstatus=1;
int lcdtimer=0;
int nowview=0;
int noF=0;
char lastComm;
String out_data="",in_data="";

int su[7]={0};
char dir='C';


menuItem menu1 = {"Driving Mode"};
menuItem menu1_1 = {"Tracer Mode",&menu1};
menuItem menu1_2 = {"Obstacle Mode",&menu1,&menu1_1};
#define Tracer_mode menu1_1.value
#define Obstacle_mode menu1_2.value

menuItem menu2 = {"Car Setting",0,&menu1};
menuItem menu2_1 = {"Light Mode",&menu2};
menuItem menu2_2 = {"Obstacle Range",&menu2,&menu2_1};
menuItem menu2_3 = {"Maximum Speed",&menu2,&menu2_2};
#define Light_mode menu2_1.value
#define Obstacle_range menu2_2.value
#define Maximum_speed menu2_3.value

menuItem menu3 = {"Fps Menu",0,&menu2};
menuItem menu3_1 = {"Add Fps",&menu3};
menuItem menu3_2 = {"Re Identify Fps",&menu3,&menu3_1};
menuItem menu3_3 = {"Delect Fps",&menu3,&menu3_2};


menuItem *menuPtr; 			// menuItem형 포인터 선언

//객체 선언
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();	//lcd
Timer t;												//timer
FPS_GT511C3 fps(fps_rx, fps_tx); 						//fps (세모부분부터 1번, TX RX GND 5V)
NewPing distsens(sens_tring,sens_echo,200);
//}

int lcdSet(){		//LCD 세팅
	static byte locked[8] = {
		0b01110,
		0b10001,
		0b10001,
		0b10001,
		0b11111,
		0b11111,
		0b11111,
		0b11111
	};
	static byte unlocked[8] = {
		0b01110,
		0b10001,
		0b00001,
		0b00001,
		0b11111,
		0b11111,
		0b11111,
		0b11111
	};
	lcd.begin(16,2);
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.createChar(1, locked);
	lcd.createChar(2, unlocked);
	return 1;
}

void lcdoff(){		//LCD 화면 OFF
	lcd.setBacklight(LCD_OFF);
	lcd.noDisplay();
	lcdstatus = 0;
}

void lcdon(){		//LCD 화면 ON
	lcd.setBacklight(WHITE);
	lcd.display();
	lcdstatus = 1;
}

int menuSet(){		//메뉴 세팅
	//next child value 값만 설정
	//next 설정
	menu1.next = &menu2;
	menu2.next = &menu3;
	menu1_1.next = &menu1_2;
	menu2_1.next = &menu2_2;
	menu2_2.next = &menu2_3;
	menu3_1.next = &menu3_2;
	menu3_2.next = &menu3_3;
	//child 설정
	menu1.child = &menu1_1;
	menu2.child = &menu2_1;
	menu3.child = &menu3_1;
	//value 설정
	menu1_1.value = "Remote";
	menu1_2.value = "ON";
	menu2_1.value = "AUTO";
	menu2_2.value = "20";
	menu2_3.value = "255";
	menu3_1.value = "Sure?";
	menu3_2.value = "Sure?";
	menu3_3.value = "Sure?";
	
	menuPtr = &menu1; //최초 출력될 메뉴
	return 1;
}

int fpsSet(){		//지문인식기 세팅
	fps.Open();
	fps.SetLED(false);
	return 1;
}

int otherSet(){		//기타 세팅
	Serial3.begin(9600);
	pinMode(p_spk,OUTPUT);
	pinMode(led,OUTPUT);
	pinMode(sens_cds,INPUT_PULLUP);
	pinMode(sens_pir,INPUT);
	pinMode(LMT_pwm,OUTPUT);
	pinMode(LMT_f,OUTPUT);
	pinMode(LMT_b,OUTPUT);
	pinMode(RMT_pwm,OUTPUT);
	pinMode(RMT_f,OUTPUT);
	pinMode(RMT_b,OUTPUT);
	pinMode(bal,OUTPUT);
	Serial3.begin(9600);
	for(int i=0;i<7;i++){
		pinMode(sens_su[i],INPUT);
	}
}

void sensing(){		//센서들 센싱
	buttons = lcd.readButtons();
	if (Light_mode == "AUTO")
		light = analogRead(sens_cds);
	//pir = digitalRead(sens_pir);
	if(distsens.ping_cm() != 0)
		distance = distsens.ping_cm();
	if(distance <= atoi(Obstacle_range)){
		noF=1;
		Serial.println("NOF!");
	}
	else{
		noF=0;
	}
	if(Tracer_mode == "Tracer"){
		digitalWrite(bal,0);
		for(int i=0;i<7;i++){
			su[i] = digitalRead(sens_su[i]);
		}
		digitalWrite(bal,1);
	}
}

void communicate(){	//통신관련 함수
	if(Serial3.available()){	//Serial3 버퍼에 자료있으면
		while(Serial3.available()){	//모두 받아서 in_data에 넣음
			in_data += (char)Serial3.read(); 
		}
	}
	if (in_data[0] == 'R'){
		if (in_data[1] == 'l' || in_data[1] == 'r' || in_data[1] == 'f' || in_data[1] == 'b' || in_data[1] == 's'){	//받은자료가 l r f b s 
			Serial.print("Recevied data : ");
			Serial.println(in_data);
			remote();
			lastComm=in_data[1];
			in_data = "";
		}
	}
	if (in_data[0] == 'S'){
		char buf[10];
		for(int i=0;in_data[i+2]=='\n';i++){
			buf[i] = in_data[i+2];
		}
		//in_data.toCharArray(buf,in_data.length()-1,2);
		String tmp = in_data.substring(2);
		if (in_data[1] == 't'){
			if(tmp == "Remote"){
				Tracer_mode = "Remote";
			}
			else if(tmp == "Tracer"){
				Tracer_mode = "Tracer";
			}
		}
		else if (in_data[1] == 'l'){
			if(tmp == "ON"){
				Light_mode = "ON";
			}
			else if (tmp == "OFF"){
				Light_mode = "OFF";
			}
			else if (tmp == "AUTO"){
				Light_mode = "AUTO";
			}
		}
		else if (in_data[1] == 'o'){
			if(tmp == "ON"){
				Obstacle_mode = "ON";
			}
			else if (tmp == "OFF"){
				Obstacle_mode = "OFF";
			}
		}
		else if (in_data[1] == 'r'){
			Obstacle_range = buf;
			Serial.println(Obstacle_range);
		}
		else if (in_data[1] == 's'){
			Maximum_speed = buf;
		}
		else if (in_data[1] == 'f'){
			delFps(atoi(buf));
		}
		for(int i=0;i<5;i++){
			buf[i] = '0';
		}
	}
	in_data = "";
}

void remote(){		//원격조정 함수
	if (in_data[1] == 'l'){
		moterB('L',(int)Maximum_speed*0.6);
		moterF('R',(int)Maximum_speed*0.6);
	}
	else if (in_data[1] == 'r'){
		moterF('L',(int)Maximum_speed*0.6);
		moterB('R',(int)Maximum_speed*0.6);
	}
	else if (in_data[1] == 'f'){
		moterF('L',(int)Maximum_speed);
		moterF('R',(int)Maximum_speed);
	}
	else if (in_data[1] == 'b'){
		moterB('L',(int)Maximum_speed);
		moterB('R',(int)Maximum_speed);
	}
	else if (in_data[1] == 's'){
		moterS();
	}
}

void menu(){		//메뉴 버튼 디스플레이
	if (buttons & BUTTON_UP) {
		if (nowview==1){
			doUpdown();
			return;
		}
		else if (menuPtr->pre!=0){
			menuPtr = menuPtr->pre;
		}
	}
	else if (buttons & BUTTON_DOWN) {
		if (nowview==1){
			doUpdown();
			return;
		}
		else if (menuPtr->next!=0){
			menuPtr = menuPtr->next;
		}
	}
	else if (buttons & BUTTON_LEFT) {
		if(nowview==1);
		else if (menuPtr->parent!=0){
			menuPtr = menuPtr->parent;
		}
	}
	else if (buttons & BUTTON_RIGHT) {
		if (menuPtr->child!=0){
			menuPtr = menuPtr->child;
		}
		else if (menuPtr->value!=0){
			lcdprint(0,menuPtr->title);
			lcdprint(1,menuPtr->value);
			nowview=1;
			return;
		}
	}
	else if (buttons & BUTTON_SELECT) {
		if (nowview==1){
			doSelect();
			return;
		}
	}
	if(menuPtr->parent!=0){
		lcdprint(0,menuPtr->parent->title);
	}
	else {
		lcdprint(0,"--Menu--");
	}
	lcdprint(1,menuPtr->title);
	nowview=0;
}

void mainMenu(){	//메인메뉴 출력 함수
	if (login_user==-1){
		lcd.clear();
		lcdprint(0,"Prees any button");
		lcdprint(1,"to login user");
	}
}

void doSelect(){		//메뉴에서 Select버튼 동작
	if (menuPtr->value == "Remote"){
		menuPtr->value = "Tracer";
	}
	else if (menuPtr->value == "Tracer"){
		menuPtr->value = "Remote";
	}
	if (menuPtr->value == "ON"){
		menuPtr->value = "OFF";
	}
	else if (menuPtr->value == "OFF"){
		if(menuPtr->title=="Light Mode"){
			menuPtr->value = "AUTO";
			lcdprint(1,menuPtr->value);
			return;
		}
		menuPtr->value = "ON";
	}
	else if (menuPtr->value == "AUTO"){
		menuPtr->value = "ON";
	}
	if (menuPtr->value == "Sure?"){
		if (menuPtr->title == "Add Fps"){
			enroll();
		}
		else if (menuPtr->title == "Delete Fps"){
			if(delFps(login_user)==1){
				login_user==-1;
				mainMenu();
			}
		}
		else if (menuPtr->title == "Re Identify Fps"){
			identify();
		}
	}
	if (menuPtr->value == "No?"){
		lcdprint(0,menuPtr->parent->title);
		lcdprint(1,menuPtr->title);
		//menuPtr = menuPtr->parent;
		nowview=0;
		return;
	}
	lcdprint(1,menuPtr->value);
}

void doUpdown(){		//메뉴에서 위아래 버튼 동작
	if (menuPtr->value == "Sure?"){
		menuPtr->value = "No?";
		lcdprint(1,menuPtr->value);
	}
	else if (menuPtr->value == "No?"){
		menuPtr->value = "Sure?";
		lcdprint(1,menuPtr->value);
	}
	if (menuPtr->title == "Obstacle Range"){
		if (buttons & BUTTON_UP) {
			int range = atoi(Obstacle_range);
			if (range<100){
				itoa(++range,Obstacle_range,10);
			}
		}
		else if (buttons & BUTTON_DOWN) {
			int range = atoi(Obstacle_range);
			if (range>5) {
				itoa(--range,Obstacle_range,10);
			}
		}
	}
	else if (menuPtr->title == "Maximum Speed"){
		if (buttons & BUTTON_UP) {
			int speed = atoi(Maximum_speed);
			if (speed<=245){
				speed += 10;
				itoa(speed,Maximum_speed,10);
			}
		}
		else if (buttons & BUTTON_DOWN) {
			int speed = atoi(Maximum_speed);
			if (speed>105){
				speed -= 10;
				itoa(speed,Maximum_speed,10);
			}
		}
	}
	Serial.println(menuPtr->value);
	lcdprint(1,menuPtr->value);
}

void lcdprint(int row, char* msg){	//LCD출력(줄,내용)
	lcd.setCursor(0, row);
	int len = strlen(msg);
	int i = 0;
	lcd.print(msg);
	for(i=len-1;i<16;i++){
		lcd.print(" ");
	}
	lcd.setCursor(len, row);
}

void spk_ok(){	//OK사운드 재생
	digitalWrite(p_spk,1);
	t.after(100,spk_stop);
	if (spk++==0){
		t.after(300,spk_ok);
		return;
	}
	spk=0;
}

void spk_no(){	//NO사운드 재생
	digitalWrite(p_spk,1);
	t.after(1000,spk_stop);
}

void spk_stop(){	//스피커 OFF
	digitalWrite(p_spk,0);
}

int identify(){	//지문인식
	// Identify fingerprint test
	fps.SetLED(true);
	lcd.clear();
	lcdprint(0,"Please press");
	lcdprint(1,"finger");
	while(1){
		t.update();
		delay(50);
		if (fps.IsPressFinger()){
			fps.CaptureFinger(false);
			login_user = fps.Identify1_N();
			if (login_user <200){
				fps.SetLED(false);
				return 1;
			}
			else{
				fps.SetLED(false);
				return 0;
			}
		}
	}
}

int enroll(){	//지문등록
	fps.SetLED(true);
	int enrollid = 0;
	bool usedid = true;
	while (usedid){
		usedid = fps.CheckEnrolled(enrollid);
		if (usedid) enrollid++;
	}
	fps.EnrollStart(enrollid);
	lcdprint(0,"Press finger to");
	lcdprint(1,"User num #");
	lcd.setCursor(0,10);
	lcd.print(enrollid);
	while(fps.IsPressFinger() == false) delay(100);
	bool bret = fps.CaptureFinger(true);
	int iret = 0;
	if (bret != false)
	{
		lcd.clear();
		lcdprint(0,"Remove finger");
		fps.Enroll1(); 
		while(fps.IsPressFinger() == true) delay(100);
		lcdprint(0,"Press finger");
		lcdprint(1,"again (1/3)");
		while(fps.IsPressFinger() == false) delay(100);
		bret = fps.CaptureFinger(true);
		if (bret != false)
		{
			lcdprint(0,"Remove finger");
			fps.Enroll2();
			while(fps.IsPressFinger() == true) delay(100);
			lcdprint(0,"Press finger");
			lcdprint(1,"again (2/3)");
			while(fps.IsPressFinger() == false) delay(100);
			bret = fps.CaptureFinger(true);
			if (bret != false)
			{
				Serial.println("Remove finger");
				iret = fps.Enroll3();
				if (iret == 0)
				{
					lcdprint(0,"Enrolling");
					lcdprint(1,"Successfull");
					fps.SetLED(false);
					spk_ok();
					return 1;
				}
				else
				{
					lcdprint(0,"Enrolling Failed");
					lcdprint(1,"code:");
					lcd.setCursor(0,5);
					lcd.print(iret);
					spk_no();
				}
			}
			else Serial.println("Failed capture third finger"); spk_no();
		}
		else Serial.println("Failed capture second finger"); spk_no();
	}
	else Serial.println("Failed capture first finger"); spk_no();
	fps.SetLED(false);
	return 0;
}

int delFps(int id){	//지문삭제
	if (fps.CheckEnrolled(id)==0){
		lcdprint(0,"Not Found #");
		lcd.setCursor(0,11);
		lcd.print(id);
		spk_no();
		return 0;
	}
	fps.DeleteID(id);
	Serial.println("Deleted #");
	lcd.setCursor(0,9);
	lcd.print(id);
	spk_ok();
	return 1;
}

void lineTracer(){	//라인트레이서
	if(su[1] == NOLINE && su[2] == NOLINE && su[3] == NOLINE && su[4] == NOLINE && su[5] == NOLINE){
			if(dir=='C'){
				moterS();
			}
			else if(dir=='L'){
				moterF('L',atoi(Maximum_speed)*Crv4);
				moterF('R',atoi(Maximum_speed)*Crv1);
			}
			else if(dir=='R') {
				moterF('L',atoi(Maximum_speed)*Crv1);
				moterF('R',atoi(Maximum_speed)*Crv4);
			}
		}
		if(su[2]==LINE && su[3]==LINE && su[4]==LINE){ //○●●●○
			moterF('L',atoi(Maximum_speed));
			moterF('R',atoi(Maximum_speed));
			dir = 'C';
		}
		else if(su[1]==LINE && su[2]==LINE && su[3]==LINE){ //●●●○○
			moterF('L',atoi(Maximum_speed)*Crv2);
			moterF('R',atoi(Maximum_speed)*Crv1);
			dir = 'L';
		}
		else if(su[3]==LINE && su[4]==LINE && su[5]==LINE){ //○○●●●
			moterF('L',atoi(Maximum_speed)*Crv1);
			moterF('R',atoi(Maximum_speed)*Crv2);
			dir = 'R';
		}
		else if(su[3]==LINE){
			if(su[4]==LINE){			// ○○●●○
				moterF('L',atoi(Maximum_speed)*Crv1);
				moterF('R',atoi(Maximum_speed)*Crv2);
				dir = 'R';
			}
			else if(su[2]==LINE){	// ○●●○○
				moterF('L',atoi(Maximum_speed)*Crv2);
				moterF('R',atoi(Maximum_speed)*Crv1);
				dir = 'L';
			}
			else{						// ○○●○○
				moterF('L',atoi(Maximum_speed));
				moterF('R',atoi(Maximum_speed));
				dir = 'C';
			}
		}
		else if(su[4]==LINE){
			if(su[5]==LINE){ 		// ○○○●●
				moterF('L',atoi(Maximum_speed)*Crv1);
				moterF('R',atoi(Maximum_speed)*Crv4);
			}
			else{						// ○○○●○
				moterF('L',atoi(Maximum_speed)*Crv1);
				moterF('R',atoi(Maximum_speed)*Crv3);
			}
			dir = 'R';
		}
		else if(su[2]==LINE){
			if(su[1]==LINE){ 		// ●●○○○
				moterF('L',atoi(Maximum_speed)*Crv4);
				moterF('R',atoi(Maximum_speed)*Crv1);
			}
			else{						// ○●○○○
				moterF('L',atoi(Maximum_speed)*Crv3);
				moterF('R',atoi(Maximum_speed)*Crv1);
			}
			dir = 'L';
		}
		else if(su[5]==LINE){		// ○○○○●
			moterF('L',atoi(Maximum_speed)*Crv1);
			moterF('R',atoi(Maximum_speed)*Crv4);
			dir = 'R';
		}
		else if(su[1]==LINE){		// ●○○○○
			moterF('L',atoi(Maximum_speed)*Crv4);
			moterF('R',atoi(Maximum_speed)*Crv1);
			dir = 'L';
		}
		else {
			moterS();
		}
}

void moterF(char dir,int spd){	//모터 전진(방향,속도)
	if(dir == 'L'){
		digitalWrite(LMT_f,1);
		digitalWrite(LMT_b,0);
		analogWrite(LMT_pwm,spd);
	}
	else{
		digitalWrite(RMT_f,1);
		digitalWrite(RMT_b,0);
		analogWrite(RMT_pwm,spd);
	}
}

void moterB(char dir,int spd){	//모터 후진(방향,속도)
	if(dir == 'L'){
		digitalWrite(LMT_f,0);
		digitalWrite(LMT_b,1);
		analogWrite(LMT_pwm,spd);
	}
	else{
		digitalWrite(RMT_f,0);
		digitalWrite(RMT_b,1);
		analogWrite(RMT_pwm,spd);
	}
}

void moterS(){	//모터 정지
	digitalWrite(LMT_f,0);
	digitalWrite(LMT_b,0);
	digitalWrite(RMT_f,0);
	digitalWrite(RMT_b,0);
	analogWrite(LMT_pwm,0);
	analogWrite(RMT_pwm,0);
}

void setup(){	//초기 세팅
	Serial.begin(9600);
	Serial.print("LCD Setting... ");
	if (lcdSet())
		Serial.println("Done!");
	Serial.print("Menu Setting... ");
	if (menuSet())
		Serial.println("Done!");
	Serial.print("FPS Setting... ");
	if (fpsSet())
		Serial.println("Done!");
	Serial.print("Other Setting... ");
	if (otherSet())
		Serial.println("Done!");
	t.every(sensingPeriod,sensing);
	t.every(50,communicate);
	Serial.println("Ready to start!");
	lcdprint(0,"Ready to start!");
	t.update();
	t.after(1000,mainMenu);
}

void loop(){	//루프
	t.update();
	if (lcdstatus==1 && lcdtimer==0){
		lcdtimer = t.after(10000,lcdoff);
	}
	if (buttons) {
		if (lastBTN!=buttons){
			t.stop(lcdtimer);
			lcdtimer = 0;
			lastBTN = buttons;
			if (lcdstatus==0){
				lcdon();
			}
			else if (login_user==-1){
				if(identify()==1){
					lcd.clear();
					lcd.setCursor(1,0);
					lcd.write(2);
					lcdprint(1,"Verified ID #");
					lcd.print(login_user);
					spk_ok();
					t.after(1000,menu);
				}
				else {
					lcd.clear();
					lcd.setCursor(1,0);
					lcd.write(1);
					lcdprint(1,"Access Denied");
					spk_no();
					login_user = -1;
					t.after(1000,mainMenu);
				}
			 }
			else {
				menu();
			}
		}
	}
	else if (!buttons)
		lastBTN = 0;
	if (Light_mode == "AUTO"){
		if(light>=cert_light){
			digitalWrite(led,HIGH);
		}
		else
			digitalWrite(led,0);
	}
	else if (Light_mode == "ON"){
		digitalWrite(led,1);
	}
	else if (Light_mode == "OFF"){
		digitalWrite(led,0);
	}
	if (Obstacle_mode == "ON" && noF && lastComm == 'f'){
		moterS();
		Serial.print("Too close! ( ");
		Serial.print(distance);
		Serial.println("cm )");
	}
	if (Tracer_mode=="Tracer"){
		if (!noF){
			lineTracer();
		}
	}
}