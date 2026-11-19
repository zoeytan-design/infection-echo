/*
  衡評述值 Pricetory — RFID 讀卡機程式（只管 RFID）
  ------------------------------------------------------------
  以 MFRC522 函式庫的官方範例（DumpInfo / ReadNUID）為基礎改寫，
  只用官方範例裡的指令，沒有額外調整天線或重置方式。

  傳給電腦的訊息（每一行一個）：
    READY             開機完成
    TAG:04A1B2C3      卡片放上（卡號）
    TAG_OFF:04A1B2C3  卡片拿起
    DBG ... / INFO .. 給人看的狀況說明（主機程式會忽略）

  ===== 接線（UNO）=====
    RC522        UNO
    SDA(SS)  →   D10
    SCK      →   D13
    MOSI     →   D11
    MISO     →   D12
    RST      →   D6      ← 跟官方範例測試時一樣，接哪隻就改下面的 RST_PIN
    3.3V     →   3.3V    ⚠ 一定要 3.3V，接 5V 會燒掉
    GND      →   GND
    IRQ      →   不接

  ===== 需要的函式庫 =====
    工具 → 管理程式庫 → 搜尋「MFRC522」→ 安裝（作者 GithubCommunity）

  ===== 監控視窗速度：115200 =====
*/
#include <SPI.h>
#include <MFRC522.h>

// ======== 可以調整的設定 ========
#define SS_PIN   10        // RC522 的 SDA 接哪隻腳
#define RST_PIN  6         // RC522 的 RST 接哪隻腳（官方範例測試成功時填的那個）
#define DEBUG    1         // 1：印出「等待刷卡中」「刷卡成功」等說明；正式展出可以改 0
const unsigned long POLL_MS   = 100;    // 多久檢查一次卡片（毫秒）
const unsigned long REMOVE_MS = 800;    // 連續多久讀不到，才算卡片被拿起（毫秒）
// =================================

MFRC522 mfrc522(SS_PIN, RST_PIN);   // 和官方範例一樣的寫法

String currentUid = "";             // 現在放在台上的卡號（空白＝沒有卡）
unsigned long lastSeen = 0;         // 最後一次看到卡片的時間
unsigned long lastPoll = 0;
unsigned long lastReport = 0;

// 函式先宣告
bool cardPresent();
String uidString();

void setup() {
  Serial.begin(115200);
  while (!Serial);                  // 和官方範例一樣
  SPI.begin();                      // 和官方範例一樣
  mfrc522.PCD_Init();               // 和官方範例一樣
  delay(4);                         // 和官方範例一樣：有些板子初始化後要等一下
#if DEBUG
  Serial.print(F("INFO "));
  mfrc522.PCD_DumpVersionToSerial(); // 印出讀卡機版本（官方範例的指令）
  Serial.println(F("DBG RFID 準備好了，把卡片平貼在讀卡機有線圈的那一面"));
#endif
  Serial.println("READY");
}

void loop() {
  if (millis() - lastPoll < POLL_MS) return;
  lastPoll = millis();

  if (cardPresent()) {
    lastSeen = millis();
    // 讀卡號（官方範例的指令）。讀失敗就當作「卡還在」，下次再讀，不會誤判成拿起
    if (mfrc522.PICC_ReadCardSerial()) {
      String uid = uidString();
      if (uid != currentUid) {
        if (currentUid.length() > 0) { Serial.print("TAG_OFF:"); Serial.println(currentUid); }
        currentUid = uid;
#if DEBUG
        Serial.print(F("DBG RFID ✓ 刷卡成功！卡號 ")); Serial.print(uid); Serial.println(F("（把這串抄進 config.js）"));
#endif
        Serial.print("TAG:"); Serial.println(uid);
      }
    }
  } else if (currentUid.length() > 0 && millis() - lastSeen > REMOVE_MS) {
#if DEBUG
    Serial.print(F("DBG RFID 卡片拿起來了：")); Serial.println(currentUid);
#endif
    Serial.print("TAG_OFF:"); Serial.println(currentUid);
    currentUid = "";
  }

#if DEBUG
  if (millis() - lastReport > 3000) {       // 每 3 秒報告一次，確認程式有在跑
    lastReport = millis();
    if (currentUid.length() > 0) { Serial.print(F("DBG RFID 卡片在台上：")); Serial.println(currentUid); }
    else Serial.println(F("DBG RFID 等待刷卡中…"));
  }
#endif
}

// 有沒有卡片在讀卡機上
// 用官方範例的 PICC_IsNewCardPresent()。連續問兩次的原因：
// 剛被讀過的卡會停在「已選取」狀態，第一次詢問它不會回答（但會因此回到待命），第二次就會回答。
// 這樣卡片一直放著也偵測得到，拿走時才知道。
bool cardPresent() {
  if (mfrc522.PICC_IsNewCardPresent()) return true;
  return mfrc522.PICC_IsNewCardPresent();
}

// 把卡號轉成文字，例如 04A1B2C3
String uidString() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}
