#include <iostream>
#include <pqxx/pqxx>
#include <string>

void showMenu() {
    std::cout << "\n=== БИБЛИОТЕЧНАЯ СИСТЕМА ===\n";
    std::cout << "=== ПРОСМОТР ===\n";
    std::cout << "1. Все книги\n";
    std::cout << "2. Все читатели\n";
    std::cout << "3. Все жанры\n";
    std::cout << "4. Все авторы\n";

    std::cout << "\n=== ПОИСК ===\n";
    std::cout << "5. Книги по автору\n";
    std::cout << "6. Книги по жанру\n";
    std::cout << "7. Книги на руках\n";

    std::cout << "\n=== ОПЕРАЦИИ ===\n";
    std::cout << "8. Выдать книгу\n";
    std::cout << "9. Вернуть книгу\n";
    std::cout << "10. Добавить книгу\n";
    std::cout << "11. Добавить читателя\n";

    std::cout << "\n=== АНАЛИТИКА ===\n";
    std::cout << "12. Авторы с >1 книгой (GROUP BY + HAVING)\n";
    std::cout << "13. Самые старые книги (подзапрос)\n";
    std::cout << "14. Все авторы (LEFT JOIN)\n";
    std::cout << "15. Статистика\n";

    std::cout << "\n=== SQL-ИНЪЕКЦИИ (ДЕМО) ===\n";
    std::cout << "16. Уязвимый поиск\n";
    std::cout << "17. UNION-инъекция\n";
    std::cout << "18. Уязвимое удаление\n";

    std::cout << "\n0. Выход\n";
    std::cout << "Выбор: ";
}

int main() {
    try {
        pqxx::connection conn(
            "host=localhost port=5432 dbname=library_db "
            "user=virtualbox password=123"
        );

        if (!conn.is_open()) {
            std::cout << "Ошибка подключения!" << std::endl;
            return 1;
        }

        std::cout << "Подключение к PostgreSQL успешно!" << std::endl;

        int choice;
        do {
            showMenu();
            std::cin >> choice;
            std::cin.ignore();

            pqxx::work txn(conn);

            switch(choice) {
                case 1: {
                    std::cout << "\n=== Все книги ===\n";
                    pqxx::result res = txn.exec(
                        "SELECT b.book_id, b.title, a.name as author, "
                        "g.name as genre, b.year_published "
                        "FROM books b "
                        "JOIN authors a ON b.author_id = a.author_id "
                        "JOIN genres g ON b.genre_id = g.genre_id "
                        "ORDER BY b.book_id"
                    );

                    for (const auto& row : res) {
                        std::cout << "ID:" << row["book_id"].as<int>()
                                  << " «" << row["title"].c_str() << "»\n"
                                  << "   Автор: " << row["author"].c_str()
                                  << ", Жанр: " << row["genre"].c_str()
                                  << ", Год: " << row["year_published"].as<int>() << "\n";
                    }
                    std::cout << "Всего: " << res.size() << " книг\n";
                    break;
                }

                case 2: {
                    std::cout << "\n=== Все читатели ===\n";
                    pqxx::result res = txn.exec(
                        "SELECT reader_id, full_name, email, phone "
                        "FROM readers ORDER BY reader_id"
                    );

                    for (const auto& row : res) {
                        std::cout << "ID:" << row["reader_id"].as<int>()
                                  << " " << row["full_name"].c_str() << "\n"
                                  << "   Email: " << row["email"].c_str()
                                  << ", Телефон: " << row["phone"].c_str() << "\n";
                    }
                    std::cout << "Всего: " << res.size() << " читателей\n";
                    break;
                }

                case 3: {
                    std::cout << "\n=== Все жанры ===\n";
                    pqxx::result res = txn.exec(
                        "SELECT g.genre_id, g.name, COUNT(b.book_id) as book_count "
                        "FROM genres g "
                        "LEFT JOIN books b ON g.genre_id = b.genre_id "
                        "GROUP BY g.genre_id, g.name "
                        "ORDER BY g.genre_id"
                    );

                    for (const auto& row : res) {
                        std::cout << "ID:" << row["genre_id"].as<int>()
                                  << " " << row["name"].c_str()
                                  << " (" << row["book_count"].as<int>() << " книг)\n";
                    }
                    break;
                }

                case 4: {
                    std::cout << "\n=== Все авторы ===\n";
                    pqxx::result res = txn.exec(
                        "SELECT a.author_id, a.name, a.country, "
                        "COUNT(b.book_id) as book_count "
                        "FROM authors a "
                        "LEFT JOIN books b ON a.author_id = b.author_id "
                        "GROUP BY a.author_id, a.name, a.country "
                        "ORDER BY a.author_id"
                    );

                    for (const auto& row : res) {
                        std::cout << "ID:" << row["author_id"].as<int>()
                                  << " " << row["name"].c_str()
                                  << " (" << row["country"].c_str() << ")"
                                  << " - " << row["book_count"].as<int>() << " книг\n";
                    }
                    break;
                }

                case 5: {
                    std::cout << "Введите имя автора: ";
                    std::string authorName;
                    std::getline(std::cin, authorName);

                    pqxx::result res = txn.exec_params(
                        "SELECT b.book_id, b.title, b.year_published "
                        "FROM books b "
                        "JOIN authors a ON b.author_id = a.author_id "
                        "WHERE a.name ILIKE '%' || $1 || '%' "
                        "ORDER BY b.year_published",
                        authorName
                    );

                    std::cout << "\n=== Книги автора \"" << authorName << "\" ===\n";
                    if (res.size() == 0) {
                        std::cout << "Не найдено\n";
                    } else {
                        for (const auto& row : res) {
                            std::cout << "ID:" << row["book_id"].as<int>()
                                      << " «" << row["title"].c_str() << "»"
                                      << " (" << row["year_published"].as<int>() << ")\n";
                        }
                    }
                    break;
                }

                case 6: {
                    std::cout << "Введите жанр: ";
                    std::string genre;
                    std::getline(std::cin, genre);

                    pqxx::result res = txn.exec_params(
                        "SELECT b.book_id, b.title, a.name as author, b.year_published "
                        "FROM books b "
                        "JOIN authors a ON b.author_id = a.author_id "
                        "JOIN genres g ON b.genre_id = g.genre_id "
                        "WHERE g.name = $1 "
                        "ORDER BY b.title",
                        genre
                    );

                    std::cout << "\n=== Книги жанра \"" << genre << "\" ===\n";
                    if (res.size() == 0) {
                        std::cout << "Не найдено\n";
                    } else {
                        for (const auto& row : res) {
                            std::cout << "ID:" << row["book_id"].as<int>()
                                      << " «" << row["title"].c_str() << "»"
                                      << " (" << row["author"].c_str()
                                      << ", " << row["year_published"].as<int>() << ")\n";
                        }
                    }
                    break;
                }

                case 7: {
                    std::cout << "\n=== Книги на руках ===\n";
                    pqxx::result res = txn.exec(
                        "SELECT br.borrowing_id, b.book_id, b.title, "
                        "r.reader_id, r.full_name as reader, br.borrow_date "
                        "FROM borrowings br "
                        "JOIN books b ON br.book_id = b.book_id "
                        "JOIN readers r ON br.reader_id = r.reader_id "
                        "WHERE br.returned = false "
                        "ORDER BY br.borrow_date"
                    );

                    if (res.size() == 0) {
                        std::cout << "Все книги в библиотеке\n";
                    } else {
                        for (const auto& row : res) {
                            std::cout << "Выдача ID:" << row["borrowing_id"].as<int>() << "\n"
                                      << "   Книга ID:" << row["book_id"].as<int>()
                                      << " «" << row["title"].c_str() << "»\n"
                                      << "   Читатель ID:" << row["reader_id"].as<int>()
                                      << " " << row["reader"].c_str() << "\n"
                                      << "   Выдана: " << row["borrow_date"].as<std::string>() << "\n";
                        }
                    }
                    std::cout << "Всего на руках: " << res.size() << " книг\n";
                    break;
                }

                case 8: {
                    std::cout << "\n=== Выдача книги ===\n";
                    int bookId, readerId;

                    std::cout << "ID книги: ";
                    std::cin >> bookId;
                    std::cout << "ID читателя: ";
                    std::cin >> readerId;
                    std::cin.ignore();

                    // Проверка существования
                    pqxx::result bookCheck = txn.exec_params(
                        "SELECT COUNT(*) FROM books WHERE book_id = $1", bookId);
                    pqxx::result readerCheck = txn.exec_params(
                        "SELECT COUNT(*) FROM readers WHERE reader_id = $1", readerId);

                    if (bookCheck[0][0].as<int>() == 0) {
                        std::cout << "Книга не найдена!\n";
                        break;
                    }
                    if (readerCheck[0][0].as<int>() == 0) {
                        std::cout << "Читатель не найден!\n";
                        break;
                    }

                    // Проверка доступности
                    pqxx::result availableCheck = txn.exec_params(
                        "SELECT COUNT(*) FROM borrowings "
                        "WHERE book_id = $1 AND returned = false",
                        bookId);

                    if (availableCheck[0][0].as<int>() > 0) {
                        std::cout << "Книга уже выдана!\n";
                        break;
                    }

                    // Выдача
                    txn.exec_params(
                        "INSERT INTO borrowings (book_id, reader_id, returned) "
                        "VALUES ($1, $2, false)",
                        bookId, readerId);

                    std::cout << "Книга выдана!\n";
                    break;
                }

                case 9: {
                    std::cout << "\n=== Возврат книги ===\n";
                    int borrowingId;
                    std::cout << "ID выдачи: ";
                    std::cin >> borrowingId;
                    std::cin.ignore();

                    txn.exec_params(
                        "UPDATE borrowings SET returned = true, return_date = CURRENT_DATE "
                        "WHERE borrowing_id = $1 AND returned = false",
                        borrowingId);

                    std::cout << "Книга возвращена!\n";
                    break;
                }

                case 10: {
                    std::cout << "\n=== Добавление книги ===\n";
                    std::string title, authorName, genreName;
                    int year, pages;

                    std::cout << "Название: ";
                    std::getline(std::cin, title);
                    std::cout << "Автор: ";
                    std::getline(std::cin, authorName);
                    std::cout << "Жанр: ";
                    std::getline(std::cin, genreName);
                    std::cout << "Год: ";
                    std::cin >> year;
                    std::cout << "Страниц: ";
                    std::cin >> pages;
                    std::cin.ignore();

                    // Автор
                    pqxx::result authorRes = txn.exec_params(
                        "SELECT author_id FROM authors WHERE name = $1", authorName);
                    int authorId;
                    if (authorRes.size() == 0) {
                        txn.exec_params("INSERT INTO authors (name, country) VALUES ($1, 'Неизвестно')", authorName);
                        authorRes = txn.exec_params("SELECT author_id FROM authors WHERE name = $1", authorName);
                        authorId = authorRes[0][0].as<int>();
                    } else {
                        authorId = authorRes[0][0].as<int>();
                    }

                    // Жанр
                    pqxx::result genreRes = txn.exec_params(
                        "SELECT genre_id FROM genres WHERE name = $1", genreName);
                    int genreId;
                    if (genreRes.size() == 0) {
                        txn.exec_params("INSERT INTO genres (name) VALUES ($1)", genreName);
                        genreRes = txn.exec_params("SELECT genre_id FROM genres WHERE name = $1", genreName);
                        genreId = genreRes[0][0].as<int>();
                    } else {
                        genreId = genreRes[0][0].as<int>();
                    }

                    // Книга
                    pqxx::result bookRes = txn.exec_params(
                        "INSERT INTO books (title, author_id, genre_id, year_published, pages) "
                        "VALUES ($1, $2, $3, $4, $5) RETURNING book_id",
                        title, authorId, genreId, year, pages);

                    int bookId = bookRes[0][0].as<int>();
                    std::cout << "Книга добавлена! ID: " << bookId << "\n";
                    break;
                }

                case 11: {
                    std::cout << "\n=== Добавление читателя ===\n";
                    std::string name, email, phone;

                    std::cout << "ФИО: ";
                    std::getline(std::cin, name);
                    std::cout << "Email: ";
                    std::getline(std::cin, email);
                    std::cout << "Телефон: ";
                    std::getline(std::cin, phone);

                    pqxx::result readerRes = txn.exec_params(
                        "INSERT INTO readers (full_name, email, phone) "
                        "VALUES ($1, $2, $3) RETURNING reader_id",
                        name, email, phone);

                    int readerId = readerRes[0][0].as<int>();
                    std::cout << "Читатель добавлен! ID: " << readerId << "\n";
                    break;
                }

                case 12: {
                    std::cout << "\n=== Авторы с несколькими книгами ===\n";
                    pqxx::result res = txn.exec(
                        "SELECT a.name, a.country, COUNT(b.book_id) as book_count "
                        "FROM authors a "
                        "JOIN books b ON a.author_id = b.author_id "
                        "GROUP BY a.name, a.country "
                        "HAVING COUNT(b.book_id) > 1 "
                        "ORDER BY book_count DESC"
                    );

                    if (res.size() == 0) {
                        std::cout << "Нет таких авторов\n";
                    } else {
                        for (const auto& row : res) {
                            std::cout << row["name"].c_str()
                                      << " (" << row["country"].c_str() << ")"
                                      << " - " << row["book_count"].as<int>() << " книги\n";
                        }
                    }
                    break;
                }

                case 13: {
                    std::cout << "\n=== Самые старые книги авторов ===\n";
                    pqxx::result res = txn.exec(
                        "SELECT a.name, b.title, b.year_published "
                        "FROM books b "
                        "JOIN authors a ON b.author_id = a.author_id "
                        "WHERE b.year_published = ("
                        "    SELECT MIN(year_published) "
                        "    FROM books b2 "
                        "    WHERE b2.author_id = b.author_id"
                        ") "
                        "ORDER BY b.year_published"
                    );

                    for (const auto& row : res) {
                        std::cout << row["name"].c_str()
                                  << ": «" << row["title"].c_str() << "»"
                                  << " (" << row["year_published"].as<int>() << ")\n";
                    }
                    break;
                }

                case 14: {
                    std::cout << "\n=== Все авторы (даже без книг) ===\n";
                    pqxx::result res = txn.exec(
                        "SELECT a.name, a.country, "
                        "COUNT(b.book_id) as book_count "
                        "FROM authors a "
                        "LEFT JOIN books b ON a.author_id = b.author_id "
                        "GROUP BY a.name, a.country "
                        "ORDER BY book_count DESC"
                    );

                    for (const auto& row : res) {
                        std::cout << row["name"].c_str()
                                  << " (" << row["country"].c_str() << ")"
                                  << " - " << row["book_count"].as<int>() << " книг\n";
                    }
                    break;
                }

                case 15: {
                    std::cout << "\n=== Статистика ===\n";
                    pqxx::result stats = txn.exec(
                        "SELECT "
                        "(SELECT COUNT(*) FROM books) as books, "
                        "(SELECT COUNT(*) FROM authors) as authors, "
                        "(SELECT COUNT(*) FROM readers) as readers, "
                        "(SELECT COUNT(*) FROM genres) as genres, "
                        "(SELECT COUNT(*) FROM borrowings WHERE returned = false) as borrowed, "
                        "(SELECT AVG(pages)::numeric(10,1) FROM books) as avg_pages"
                    );

                    auto row = stats[0];
                    std::cout << "Книги: " << row["books"].as<int>() << "\n";
                    std::cout << "Авторы: " << row["authors"].as<int>() << "\n";
                    std::cout << "Читатели: " << row["readers"].as<int>() << "\n";
                    std::cout << "Жанры: " << row["genres"].as<int>() << "\n";
                    std::cout << "На руках: " << row["borrowed"].as<int>() << "\n";
                    std::cout << "Средний объем: " << row["avg_pages"].as<std::string>() << " стр.\n";
                    break;
                }

                case 16: {
                    std::cout << "\n=== Демонстрация SQL-инъекции: Уязвимый поиск ===\n";
                    std::cout << "Введите название книги: ";
                    std::string searchTitle;
                    std::getline(std::cin, searchTitle);

                    // Уязвимый запрос!
                    std::string vulnerableSQL =
                        "SELECT b.title, a.name as author, b.year_published "
                        "FROM books b "
                        "JOIN authors a ON b.author_id = a.author_id "
                        "WHERE b.title LIKE '%" + searchTitle + "%'";

                    std::cout << "Запрос: " << vulnerableSQL << "\n\n";

                    try {
                        pqxx::result res = txn.exec(vulnerableSQL);

                        if (res.size() == 0) {
                            std::cout << "Не найдено\n";
                        } else {
                            for (const auto& row : res) {
                                std::cout << "• " << row["title"].c_str()
                                          << " (" << row["author"].c_str()
                                          << ", " << row["year_published"].as<int>() << ")\n";
                            }
                        }

                        std::cout << "\nПример инъекции: ' OR '1'='1\n";
                        std::cout << "Покажет все книги!\n";

                    } catch (const std::exception& e) {
                        std::cout << "Ошибка: " << e.what() << std::endl;
                    }
                    break;
                }

                case 17: {
                    std::cout << "\n=== Демонстрация SQL-инъекции: UNION-based ===\n";
                    std::cout << "ID жанров:\n";

                    // Сначала покажем ID жанров
                    pqxx::result genres = txn.exec("SELECT genre_id, name FROM genres ORDER BY genre_id");
                    for (const auto& row : genres) {
                        std::cout << "ID " << row["genre_id"].as<int>()
                                  << " - " << row["name"].c_str() << "\n";
                    }

                    std::cout << "\nВведите ID жанра: ";
                    std::string genreId;
                    std::getline(std::cin, genreId);

                    // Уязвимый UNION-запрос
                    std::string unionSQL =
                        "SELECT title, year_published FROM books "
                        "WHERE genre_id = " + genreId;

                    std::cout << "Запрос: " << unionSQL << "\n\n";
                    std::cout << "Пример инъекции: 1 UNION SELECT email, phone FROM readers --\n";
                    std::cout << "Покажет email и телефоны читателей!\n\n";

                    try {
                        pqxx::result res = txn.exec(unionSQL);

                        if (res.size() == 0) {
                            std::cout << "Не найдено\n";
                        } else {
                            for (const auto& row : res) {
                                std::cout << "| " << row[0].c_str() << " | " << row[1].c_str() << " |\n";
                            }
                        }
                    } catch (const std::exception& e) {
                        std::cout << "Ошибка: " << e.what() << std::endl;
                    }
                    break;
                }

                case 18: {
                    std::cout << "\n=== Демонстрация SQL-инъекции: Уязвимое удаление ===\n";
                    std::cout << "Введите ID книги для удаления из выдачи: ";
                    std::string bookId;
                    std::getline(std::cin, bookId);

                    std::string dangerousSQL =
                        "DELETE FROM borrowings WHERE book_id = " + bookId;

                    std::cout << "Запрос: " << dangerousSQL << "\n";
                    std::cout << "Пример инъекции: 1 OR 1=1\n";
                    std::cout << "Удалит все записи о выдаче!\n";
                    std::cout << "\n[ДЕМО] В реальной системе запрос был бы выполнен.\n";
                    std::cout << "Защита: используйте параметризованные запросы!\n";
                    break;
                }

                case 0:
                    std::cout << "\nВыход...\n";
                    break;

                default:
                    std::cout << "Неверный выбор!\n";
            }

            if (choice != 0) {
                txn.commit();
            }

        } while (choice != 0);

    } catch (const std::exception &e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
