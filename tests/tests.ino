struct Bar {
  Bar(const char* s)
    : str(s) {}
  ~Bar() {
    Serial.print(str);
    Serial.println(" destroyed");
  }
  const char* str;
};

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("-------------------------");
  auto const& str1 = String("hello") + "!";
  auto const str2 = String("world");
  Serial.println(str1);
  Serial.println(str2);

  auto const& bar1 = Bar("hello");
  Serial.println("-- 1");
  auto const bar2 = Bar("world");
  Serial.println("-- 2");
}

void loop() {
}
