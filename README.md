# [Krafton Jungle Week08] Web server 구현

- 📅 진행 기간 : 2022. 11. 30. ~ 2022. 12. 07.
- 📃 과제 설명 : GUIDELINES.md
- 📭 개발 일지 : [Krafton Jungle | TIL_22.12.05 - 07] Malloc Lab 구현
- 🗂 함수 설명 관련 도큐먼트 : Jungle olive-su | malloc-Lab © doxygen
- 📖 참고 자료 : cmu_malloclab



---

###  🎉 Test Result

#### echo server

- gcc setting
   ```
      gcc -Wall -Werror -o echoserveri echoserveri.c csapp.c
   ```

- input

   ```
      ./echoserveri <port>
   ```

<br/>

- output

   ```js
      Connected to (localhost, 52176)
      server received 13 bytes
      server received 25 bytes
      server received 10 bytes
      server received 8 bytes
   ```

<br/>

#### echo client

- gcc setting
   ```
      gcc -Wall -Werror -o echoclient echoclient.c csapp.c
   ```

- input
   
   ```js
      ./echoclient <ip> <port>
   ```

- output

   ```
      hello jungle
      hello jungle
      this is echo server test
      this is echo server test
      you good?
      you good?
      안녕?
      안녕?
   ```

