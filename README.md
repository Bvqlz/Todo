# TaskFlow - C++ REST API Todo Application

A full-stack todo list application built with C++ backend using the Crow framework and a responsive JavaScript frontend. This project demonstrates CRUD operations, user authentication, and modern web development practices using C++ for server-side development.

## Features

### Authentication System
- **User Registration & Login** with secure password hashing (Argon2)
- **Session Management** using HTTP cookies
- **Protected Routes** requiring authentication
- **Automatic Session Validation** on page load

### Task Management (CRUD Operations)
- **Create** new tasks with descriptions
- **Read** all tasks with filtering capabilities
- **Update** task descriptions and status
- **Delete** tasks with confirmation dialogs
- **Status Tracking**: Todo, In Progress, Completed

### Frontend
- **Responsive Design** - works on desktop, tablet, and mobile
- **Task Statistics** showing task counts by status
- **Filter System** to view tasks by status
- **Modal Interface** for detailed task editing
- **Loading States** and error handling

## Technologies Used

### Backend (C++)
- **[Crow Framework](https://crowcpp.org/)** - Fast and easy to use C++ micro web framework
- **[libpqxx](https://pqxx.org/)** - PostgreSQL C++ library for database operations
- **[libsodium](https://libsodium.org/)** - Modern cryptographic library for password hashing

### Database
- **[PostgreSQL](https://www.postgresql.org/)** - Relational database
- **Foreign Key Constraints** for data integrity
- **Prepared Statements** for SQL injection protection

### Frontend
- **Vanilla JavaScript (ES6+)** - Modern JavaScript with async/await, template literals
- **HTML5** - Semantic markup with accessibility features
- **CSS3** - Custom properties, Grid, Flexbox, animations
- **[Font Awesome](https://fontawesome.com/)** - Icons for enhanced UX

### Architecture
- **REST API** - Following RESTful principles for HTTP endpoints
- **Single Page Application** - Content loading without page refreshes
- **Session-based Authentication** - Server-side session management
  
## API Endpoints

### Authentication Routes
```
POST   /register          - Create new user account
POST   /login             - User authentication
POST   /logout            - End user session
GET    /me                - Get current user info
```

### Task Management Routes
```
GET    /tasks             - Retrieve all user tasks
POST   /tasks             - Create new task
GET    /tasks/{id}        - Get specific task
PUT    /tasks/{id}        - Update existing task
DELETE /tasks/{id}        - Delete task
```

### Static File Routes
```
GET    /                  - Serve main HTML page
GET    /frontend/style.css - Serve CSS stylesheet
GET    /frontend/script.js - Serve JavaScript application
```

## Building and Running

### Prerequisites

**Note:** This project requires several external dependencies and environment setup that may make it challenging to run locally without proper configuration.

#### Required Dependencies:
- **CMake 3.31+**
- **C++20 compatible compiler** (GCC 10+, Clang 12+, MSVC 19.29+)
- **PostgreSQL** database server
- **vcpkg** package manager (recommended)

#### External Libraries (via vcpkg):
```bash
vcpkg install crow
vcpkg install libpqxx
vcpkg install libsodium
```

### Environment Variables

The application requires these environment variables for database connection:
```bash
DBNAME=your_database_name
USER=your_database_user
PASSWORD=your_database_password
HOST=localhost
PORT=5432
```

### Database Setup

1. **Create PostgreSQL database**:
```sql
CREATE DATABASE todo_app;
```

2. **The application automatically creates required tables**:
   - `users` table for user accounts
   - `tasks` table for todo items with foreign key to users

### Build Instructions

```bash
# Clone the repository
git clone [your-repo-url]
cd Todo

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake

# Build the project
cmake --build .

# Run the application
./Todo
```

## Current Limitations

### Security Considerations
- **Cookie-based Authentication**: Uses HTTP cookies for session management, which may be vulnerable to certain attacks
- **Session Storage**: Sessions are stored in memory rather than a persistent store
- **Password Hashing**: Despite using libsodium, the implementation could be enhanced with additional security measures
- **No HTTPS**: Currently runs on HTTP; production deployment should use HTTPS
- **Input Validation**: Limited server-side input validation beyond basic SQL injection protection

### Architecture Limitations  
- **Single Server**: No load balancing or horizontal scaling support
- **In-Memory Sessions**: Sessions don't persist across server restarts
- **No Database Migrations**: Schema changes require manual database updates
- **Limited Error Handling**: Some edge cases in error handling could be improved
- **No Rate Limiting**: API endpoints lack rate limiting for abuse prevention

### Development Environment
- **Local Database Required**: Requires PostgreSQL installation and configuration
- **Environment Variables**: Database credentials must be set in development environment

## Learning Outcome

This project demonstrates:

1. **C++ Web Development** using the Crow framework
2. **RESTful API Design** with HTTP methods and status codes
3. **Database Integration** with PostgreSQL and prepared statements
4. **Authentication Systems** with password hashing
5. **Frontend-Backend Communication** using fetch API and JSON
6. **Responsive Web Design** with modern CSS techniques
7. **Security Practices** like XSS prevention and input validation
8. **Project Organization** with modular code structure

---
