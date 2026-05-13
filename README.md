# Multithreaded Producer-Consumer Task Queue (C)

Ένα σύστημα διαχείρισης εργασιών (Task Queue) σε γλώσσα C, που χρησιμοποιεί το πρότυπο **Producer-Consumer**. Το project υλοποιεί μια thread-safe κυκλική ουρά με χρήση POSIX Threads (pthreads) για τον συγχρονισμό και τον υπολογισμό τριγωνομετρικών τιμών.

## Δομή Αρχείων
* `prod_cons.c`: Ο κύριος κώδικας της εφαρμογής.
* `wait_times.csv`: Αρχείο που παράγεται αυτόματα με τους χρόνους αναμονής (σε μs).
* `diagrams.py`: Python script για την δημιουργεία των plots. Διαβάζει το wait_times.csv και η διαδικασία γίνεται αυτόματα.

## Οδηγίες Μεταγλώττισης & Εκτέλεσης
Για τη μεταγλώττιση απαιτείται ο `gcc` και η σύνδεση με τις βιβλιοθήκες `pthread` και `m` (math):

```bash
gcc prod_cons.c -o task_system -lpthread -lm

```
##  Εκτέλεση κώδικα
Για την εκτέλεση κώδικα απλά γράφουμε στο terminal την εντολή:

```bash

./task_system

```


