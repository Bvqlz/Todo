// ===== GLOBAL STATE MANAGEMENT =====
let currentUser = null;
let tasks = [];
let currentFilter = 'all';
let currentTaskId = null;
let isRegisterMode = false;

// ===== DOM ELEMENT REFERENCES =====
const authSection = document.getElementById('authSection');
const mainApp = document.getElementById('mainApp');
const authForm = document.getElementById('authForm');
const authMessage = document.getElementById('authMessage');
const authSubtitle = document.getElementById('authSubtitle'); // changes on login or register
const authButton = document.getElementById('authButton');
const authButtonText = document.getElementById('authButtonText');  // changes on login or register
const toggleAuthBtn = document.getElementById('toggleAuth');  // changes on login or register
const usernameInput = document.getElementById('username');
const passwordInput = document.getElementById('password');
const usernameDisplay = document.getElementById('usernameDisplay'); // changes on which user is signed in
const logoutBtn = document.getElementById('logoutBtn');
const taskInput = document.getElementById('taskInput');
const addTaskBtn = document.getElementById('addTaskBtn');
const taskList = document.getElementById('taskList');
const filterBtns = document.querySelectorAll('.filter-btn'); // firs
const taskModal = document.getElementById('taskModal');
const modalClose = document.getElementById('modalClose');
const modalCancel = document.getElementById('modalCancel');
const modalTaskId = document.getElementById('modalTaskId');
const modalTaskDescription = document.getElementById('modalTaskDescription');
const modalTaskStatus = document.getElementById('modalTaskStatus');
const updateTaskBtn = document.getElementById('updateTaskBtn');
const deleteTaskBtn = document.getElementById('deleteTaskBtn');

// ===== UTILITY FUNCTIONS =====

/**
 * Display success or error messages in the auth section
 * @param {string} message - Message to display
 * @param {string} type - 'success' or 'error'
 */
function showMessage(message, type = 'success') {
    authMessage.className = `message ${type}`;
    authMessage.innerHTML = `
    <i class="fas fa-${type === 'success' ? 'check-circle' : 'exclamation-circle'}"></i>
    ${message}
  `;
    authMessage.style.display = 'flex'; // this will have the icon and the text next each other. More importantly makes the message visible.

    // Auto-hide success/error messages after 5 seconds
    setTimeout(() => {
        authMessage.style.display = 'none';
    }, 5000);
}

/**
 * Set loading state for buttons with spinner
 * @param {HTMLElement} element - Button element
 * @param {boolean} loading - Loading state
 */
function setLoading(element, loading) {
    if (loading) {
        element.disabled = true; // we disable the button when its loading. User cannot click it
        element.innerHTML = '<div class="loading-spinner"></div> Loading...';
    } else {
        element.disabled = false;
        element.innerHTML = element.dataset.originalText || element.textContent;
    }
}

/**
 * Update task statistics counters
 */
function updateStats() {
    const todoTasks = tasks.filter(task => task.status === 'todo');
    const inprogressTasks = tasks.filter(task => task.status === 'inprogress');
    const completedTasks = tasks.filter(task => task.status === 'completed');

    document.getElementById('todoCount').textContent = todoTasks.length;
    document.getElementById('inprogressCount').textContent = inprogressTasks.length;
    document.getElementById('completedCount').textContent = completedTasks.length;
    document.getElementById('totalCount').textContent = tasks.length;
}

/**
 * Render tasks in the task list with filtering
 */
function renderTasks() {
    const filteredTasks = currentFilter === 'all' ? tasks : tasks.filter(task => task.status === currentFilter);

    // Show empty state if no tasks
    if (filteredTasks.length === 0) {
        taskList.innerHTML = `
      <div class="empty-state">
        <i class="fas fa-clipboard-list"></i>
        <h3>${currentFilter === 'all' ? 'No tasks yet' : `No ${currentFilter} tasks`}</h3>
        <p>${currentFilter === 'all' ? 'Add a task above to get started!' : `No tasks in ${currentFilter} status`}</p>
      </div>
    `;
        updateStats();
        return;
    }

    // Render task items
    taskList.innerHTML = filteredTasks.map(task => `
    <li class="task-item" data-task-id="${task.id}">
      <div class="task-status-indicator ${task.status}"></div>
      <div class="task-content">
        <span class="task-description ${task.status}">${escapeHtml(task.description)}</span>
        <span class="task-status-badge ${task.status}">${task.status}</span>
      </div>
    </li>
  `).join('');

    // Add click listeners to task items
    document.querySelectorAll('.task-item').forEach(item => {
        item.addEventListener('click', () => {
            const taskId = parseInt(item.dataset.taskId); // in our json object the backend returns everything as a string. This turns it into a number
            const task = tasks.find(t => t.id === taskId);
            if (task) openTaskModal(task);
        });
    });

    updateStats();
}

/**
 * Escape HTML to prevent XSS attacks
 * @param {string} text - Text to escape
 * @returns {string} - Escaped text
 */
function escapeHtml(text) {
    const div = document.createElement('div'); // this is to prevent running html code in our task description.
    div.textContent = text;
    return div.innerHTML;
}

// ===== AUTHENTICATION FUNCTIONS =====

/**
 * Toggle between login and register mode
 */
function toggleAuthMode() {
    isRegisterMode = !isRegisterMode;

    if (isRegisterMode) {
        authSubtitle.textContent = 'Create your account';
        authButtonText.innerHTML = '<i class="fas fa-user-plus"></i> Create Account';
        toggleAuthBtn.textContent = 'Sign in instead';
    } else {
        authSubtitle.textContent = 'Sign in to manage your tasks';
        authButtonText.innerHTML = '<i class="fas fa-sign-in-alt"></i> Sign In';
        toggleAuthBtn.textContent = 'Create one';
    }

    // Clear form fields and messages
    usernameInput.value = '';
    passwordInput.value = '';
    authMessage.style.display = 'none';
}

/**
 * Handle authentication (login or register)
 * @param {Event} e - Form submit event
 */
async function handleAuth(e) {
    e.preventDefault();

    const username = usernameInput.value.trim();
    const password = passwordInput.value.trim();

    // Validate inputs
    if (!username || !password) {
        showMessage('Please fill in all fields', 'error');
        return;
    }

    if (username.length < 3) {
        showMessage('Username must be at least 3 characters long', 'error');
        return;
    }

    if (password.length < 6) {
        showMessage('Password must be at least 6 characters long', 'error');
        return;
    }

    // Set loading state
    authButton.dataset.originalText = authButtonText.innerHTML;
    setLoading(authButton, true);

    try {
        const endpoint = isRegisterMode ? '/register' : '/login';
        const response = await fetch(endpoint, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username, password })
        });

        const data = await response.json();

        if (response.ok) {
            showMessage(data.message || (isRegisterMode ? 'Account created successfully!' : 'Welcome back!'));

            if (!isRegisterMode) {
                // Login successful - show main app
                currentUser = { username };
                showMainApp();
            } else {
                // Registration successful - switch to login mode
                toggleAuthMode();
                showMessage('Account created! Please sign in with your credentials.');
            }
        } else {
            showMessage(data.message || 'Authentication failed', 'error');
        }
    } catch (error) {
        console.error('Auth error:', error);
        showMessage('Network error. Please check your connection and try again.', 'error');
    } finally {
        setLoading(authButton, false);
    }
}

/**
 * Handle user logout
 */
async function logout() {
    try {
        const response = await fetch('/logout', { method: 'POST' });
        const data = await response.json();

        if (response.ok) {
            showMessage(data.message || 'Logged out successfully');
        } else {
            console.error('Logout failed:', data.message);
        }
    } catch (error) {
        console.error('Logout error:', error);
    } finally {
        // Always clear local state and show auth section
        currentUser = null;
        tasks = [];
        showAuthSection();
    }
}

/**
 * Show authentication section
 */
function showAuthSection() {
    authSection.style.display = 'block';
    mainApp.style.display = 'none';

    // Clear form fields
    usernameInput.value = '';
    passwordInput.value = '';
    authMessage.style.display = 'none';

    // Reset to login mode
    if (isRegisterMode) {
        toggleAuthMode();
    }
}

/**
 * Show main application
 */
function showMainApp() {
    authSection.style.display = 'none';
    mainApp.style.display = 'block';
    usernameDisplay.textContent = currentUser.username;

    // Load user's tasks
    fetchTasks();
}

// ===== TASK MANAGEMENT FUNCTIONS =====

/**
 * Fetch tasks from the server
 */
async function fetchTasks() {
    try {
        const response = await fetch('/tasks');
        const data = await response.json();

        if (response.ok) {
            tasks = data.tasks || [];
            renderTasks();
        } else if (response.status === 401) {
            // Session expired
            showAuthSection();
            showMessage(data.message || 'Session expired. Please login again.', 'error');
        } else {
            console.error('Failed to fetch tasks:', data.message);
            showMessage('Failed to load tasks. Please refresh the page.', 'error');
        }
    } catch (error) {
        console.error('Fetch tasks error:', error);
        showMessage('Network error while loading tasks. Please check your connection.', 'error');
    }
}

/**
 * Add a new task
 */
async function addTask() {
    const description = taskInput.value.trim();

    if (!description) {
        taskInput.focus();
        return;
    }

    if (description.length > 200) {
        showMessage('Task description is too long (max 200 characters)', 'error');
        return;
    }

    // Set loading state
    const originalText = addTaskBtn.innerHTML;
    setLoading(addTaskBtn, true);

    try {
        const response = await fetch('/tasks', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ description, status: 'todo' })
        });

        const data = await response.json(); // does data return everything about the task?

        if (response.ok) {
            taskInput.value = '';
            await fetchTasks(); // Refresh tasks
        } else if (response.status === 401) {
            showAuthSection();
            showMessage(data.message || 'Authentication required', 'error');
        } else {
            showMessage(data.message || 'Failed to add task', 'error');
        }
    } catch (error) {
        console.error('Add task error:', error);
        showMessage('Network error while adding task', 'error');
    } finally {
        addTaskBtn.innerHTML = originalText;
        addTaskBtn.disabled = false;
    }
}

/**
 * Update an existing task
 */
async function updateTask() {
    if (!currentTaskId) return;

    const description = modalTaskDescription.value.trim();
    const status = modalTaskStatus.value;

    if (!description) {
        modalTaskDescription.focus();
        showMessage('Task description cannot be empty', 'error');
        return;
    }

    if (description.length > 200) {
        showMessage('Task description is too long (max 200 characters)', 'error');
        return;
    }

    // Set loading state
    const originalText = updateTaskBtn.innerHTML;
    setLoading(updateTaskBtn, true);

    try {
        const response = await fetch(`/tasks/${currentTaskId}`, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ description, status })
        });

        const data = await response.json();

        if (response.ok) {
            closeTaskModal();
            await fetchTasks(); // Refresh tasks
        } else if (response.status === 401) {
            showAuthSection();
            showMessage(data.message || 'Authentication required', 'error');
            closeTaskModal();
        } else {
            showMessage(data.message || 'Failed to update task', 'error');
        }
    } catch (error) {
        console.error('Update task error:', error);
        showMessage('Network error while updating task', 'error');
    } finally {
        updateTaskBtn.innerHTML = originalText;
        updateTaskBtn.disabled = false;
    }
}

/**
 * Delete a task
 */
async function deleteTask() {
    if (!currentTaskId) return;

    // Confirm deletion
    if (!confirm('Are you sure you want to delete this task? This action cannot be undone.')) {
        return;
    }

    // Set loading state
    const originalText = deleteTaskBtn.innerHTML;
    setLoading(deleteTaskBtn, true);

    try {
        const response = await fetch(`/tasks/${currentTaskId}`, {
            method: 'DELETE'
        });

        if (response.status === 204) {
            // Success - no content returned
            closeTaskModal();
            await fetchTasks(); // Refresh tasks
        } else if (response.status === 401) {
            const data = await response.json();
            showAuthSection();
            showMessage(data.message || 'Authentication required', 'error');
            closeTaskModal();
        } else {
            const data = await response.json();
            showMessage(data.message || 'Failed to delete task', 'error');
        }
    } catch (error) {
        console.error('Delete task error:', error);
        showMessage('Network error while deleting task', 'error');
    } finally {
        deleteTaskBtn.innerHTML = originalText;
        deleteTaskBtn.disabled = false;
    }
}

// ===== MODAL FUNCTIONS =====

/**
 * Open task modal with task data
 * @param {Object} task - Task object
 */
function openTaskModal(task) {
    currentTaskId = task.id;
    modalTaskId.value = task.id;
    modalTaskDescription.value = task.description;
    modalTaskStatus.value = task.status;
    taskModal.style.display = 'flex';

    // Focus on description input after modal animation
    setTimeout(() => modalTaskDescription.focus(), 100);
}

/**
 * Close task modal
 */
function closeTaskModal() {
    taskModal.style.display = 'none';
    currentTaskId = null;

    // Clear any error messages
    authMessage.style.display = 'none';
}

/**
 * Check authentication status on page load
 */
async function checkAuthStatus() {
    try {
        const response = await fetch('/me');
        const data = await response.json();

        if (response.ok) {
            currentUser = { username: data.username };
            showMainApp();
        } else {
            showAuthSection();
        }
    } catch (error) {
        console.error('Auth check error:', error);
        showAuthSection();
    }
}

// ===== EVENT LISTENERS =====

// Authentication events
authForm.addEventListener('submit', handleAuth);
toggleAuthBtn.addEventListener('click', toggleAuthMode);
logoutBtn.addEventListener('click', logout);

// Task input events
taskInput.addEventListener('keypress', (e) => {
    if (e.key === 'Enter') {
        addTask();
    }
});

addTaskBtn.addEventListener('click', addTask);

// Filter button events
filterBtns.forEach(btn => {
    btn.addEventListener('click', () => {
        // Update active filter button
        filterBtns.forEach(b => b.classList.remove('active'));
        btn.classList.add('active');

        // Update filter and re-render tasks
        currentFilter = btn.dataset.filter;
        renderTasks();
    });
});

// Modal events
modalClose.addEventListener('click', closeTaskModal);
modalCancel.addEventListener('click', closeTaskModal);
updateTaskBtn.addEventListener('click', updateTask);
deleteTaskBtn.addEventListener('click', deleteTask);

// Close modal when clicking outside
taskModal.addEventListener('click', (e) => {
    if (e.target === taskModal) {
        closeTaskModal();
    }
});

// Keyboard shortcuts
document.addEventListener('keydown', (e) => {
    // Close modal with Escape key
    if (e.key === 'Escape' && taskModal.style.display === 'flex') {
        closeTaskModal();
    }
});

// ===== INITIALIZATION =====

/**
 * Initialize the application
 */
function initializeApp() {
    // Check authentication status
    checkAuthStatus();

    // Add fade-in animation to auth section
    authSection.style.animation = 'fadeIn 0.5s ease-in';

    // Set focus to username input
    setTimeout(() => {
        if (authSection.style.display !== 'none') {
            usernameInput.focus();
        }
    }, 500);
}

// Start the application when DOM is ready
document.addEventListener('DOMContentLoaded', initializeApp);

// Handle page visibility changes (refresh tasks when tab becomes visible)
document.addEventListener('visibilitychange', () => {
    if (!document.hidden && currentUser && mainApp.style.display !== 'none') {
        fetchTasks();
    }
});

// Handle online/offline status
window.addEventListener('online', () => {
    if (currentUser && mainApp.style.display !== 'none') {
        fetchTasks();
    }
});

window.addEventListener('offline', () => {
    showMessage('You are currently offline. Some features may not work.', 'error');
});