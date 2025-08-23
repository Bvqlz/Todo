// Global UI elements
const authSection = document.getElementById('authSection');
const taskAppSection = document.getElementById('taskAppSection');
const usernameDisplay = document.getElementById('usernameDisplay');
const authMessage = document.getElementById('authMessage'); // For auth related messages
const authTitle = document.getElementById('authTitle');
const authButton = document.getElementById('authButton');
const toggleAuthLink = document.getElementById('toggleAuth');
const usernameInput = document.getElementById('username');
const passwordInput = document.getElementById('password');

// Task related UI elements
const taskList = document.getElementById('taskList');
const newTaskDescriptionInput = document.getElementById('newTaskDescription');
const addTaskBtn = document.getElementById('addTaskBtn');
gi
// Modal related UI elements
const taskModal = document.getElementById('taskModal');
const closeButton = taskModal.querySelector('.close-button');
const modalTaskId = document.getElementById('modalTaskId');
const modalTaskDescriptionInput = document.getElementById('modalTaskDescription');
const modalTaskStatusSelect = document.getElementById('modalTaskStatus');
const updateTaskBtn = document.getElementById('updateTaskBtn');
const deleteTaskBtn = document.getElementById('deleteTaskBtn');

let currentTaskId = null; // To store the ID of the task currently in the modal
let isRegisterMode = false; // Flag to track auth mode

// Helper function to display authentication messages
function showAuthMessage(msg, isError = false) {
    authMessage.textContent = msg;
    authMessage.className = `message ${isError ? 'error' : 'success'}`;
    // Optionally hide after some time if it's a success message
    if (!isError) {
        setTimeout(() => {
            authMessage.textContent = '';
            authMessage.className = 'message';
        }, 3000); // Hide after 3 seconds
    }
}

// Function to update UI based on login status
function updateUIForAuth(loggedIn, username = '') {
    if (loggedIn) {
        authSection.style.display = 'none';
        taskAppSection.style.display = 'block';
        usernameDisplay.textContent = username;
        fetchTasks(); // Fetch tasks once logged in
    } else {
        authSection.style.display = 'flex'; // Use flex for centering login form
        taskAppSection.style.display = 'none';
        usernameDisplay.textContent = 'Guest'; // Reset
        authTitle.textContent = 'Login';
        authButton.textContent = 'Login';
        toggleAuthLink.textContent = 'Register';
        usernameInput.value = '';
        passwordInput.value = '';
        showAuthMessage('', false); // Clear any previous auth messages
        taskList.innerHTML = ''; // Clear tasks on logout
    }
}

// Check auth status on page load
async function checkAuthStatus() {
    try {
        const response = await fetch('/me');
        const data = await response.json(); // Always expect JSON
        if (response.ok) {
            updateUIForAuth(true, data.username); // Assuming /me returns { username: "..." }
        } else {
            // If /me returns 401 or other error, show message but remain on auth screen
            updateUIForAuth(false);
            showAuthMessage(data.message || 'Please log in to manage your tasks.', true);
        }
    } catch (error) {
        console.error('Error checking auth status:', error);
        updateUIForAuth(false);
        showAuthMessage('Network error: Could not connect to the server.', true);
    }
}
document.addEventListener('DOMContentLoaded', checkAuthStatus);


// --- Authentication Logic ---
toggleAuthLink.addEventListener('click', (e) => {
    e.preventDefault();
    isRegisterMode = !isRegisterMode;
    authTitle.textContent = isRegisterMode ? 'Register' : 'Login';
    authButton.textContent = isRegisterMode ? 'Register' : 'Login';
    toggleAuthLink.textContent = isRegisterMode ? 'Login' : 'Register';
    showAuthMessage('', false); // Clear message on toggle
    usernameInput.value = ''; // Clear inputs
    passwordInput.value = '';
});

authButton.addEventListener('click', async () => {
    const username = usernameInput.value.trim();
    const password = passwordInput.value.trim();

    if (!username || !password) {
        showAuthMessage('Please enter both username and password.', true);
        return;
    }

    const endpoint = isRegisterMode ? '/register' : '/login';
    try {
        const response = await fetch(endpoint, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username, password })
        });
        const data = await response.json(); // Even if error, try to parse JSON for message

        if (response.ok) {
            showAuthMessage(data.message || (isRegisterMode ? 'Registration successful!' : 'Login successful!'), false);
            usernameInput.value = ''; // Clear fields on success
            passwordInput.value = '';

            if (!isRegisterMode) { // If it was a login, update UI immediately
                updateUIForAuth(true, username);
            } else { // If it was a registration, switch to login form
                isRegisterMode = false;
                authTitle.textContent = 'Login';
                authButton.textContent = 'Login';
                toggleAuthLink.textContent = 'Register';
            }
        } else {
            showAuthMessage(data.message || data.error || 'An unknown error occurred.', true);
        }
    } catch (error) {
        console.error('Authentication error:', error);
        showAuthMessage('Network error or server unavailable.', true);
    }
});

document.getElementById('logoutBtn').addEventListener('click', async () => {
    try {
        const response = await fetch('/logout', { method: 'POST' });
        const data = await response.json(); // Always expect JSON response
        if (response.ok) {
            showAuthMessage(data.message || 'Logged out successfully!', false);
            updateUIForAuth(false);
        } else {
            console.error('Logout failed:', response.status, data.message || data.error || 'Unknown error');
            showAuthMessage(data.message || 'Failed to log out.', true);
        }
    } catch (error) {
        console.error('Error during logout:', error);
        showAuthMessage('Network error during logout.', true);
    }
});

// --- Task Management Logic ---

// Function to fetch and display tasks
async function fetchTasks() {
    try {
        const response = await fetch('/tasks');
        const data = await response.json(); // Expect JSON array of tasks or an error object

        taskList.innerHTML = ''; // Clear existing tasks

        if (response.ok) {
            if (data.tasks && data.tasks.length > 0) {
                data.tasks.forEach(task => {
                    const li = document.createElement('li');
                    li.className = 'task-item';
                    li.dataset.taskId = task.id; // Store ID for easy access

                    const statusClass = `status-${task.status}`; // Determine the status class
                    li.innerHTML = `
              <span class="task-description">${task.description}</span>
              <span class="task-status ${statusClass}">${task.status}</span>
            `;
                    li.addEventListener('click', () => openTaskModal(task)); // Make it clickable
                    taskList.appendChild(li);
                });
            } else {
                taskList.innerHTML = '<li>No tasks yet! Add one above.</li>';
            }
        } else if (response.status === 401) {
            // Unauthorized, likely session expired
            updateUIForAuth(false);
            showAuthMessage(data.message || 'Session expired. Please log in again to view tasks.', true);
        } else {
            console.error('Failed to fetch tasks:', response.status, data.message || data.error || 'Unknown error');
            taskList.innerHTML = `<li>Error loading tasks: ${data.message || data.error || 'Unknown error'}</li>`;
        }
    } catch (error) {
        console.error('Error fetching tasks:', error);
        taskList.innerHTML = '<li>Network error. Could not connect to the server to fetch tasks.</li>';
    }
}

// Add Task
addTaskBtn.addEventListener('click', async () => {
    const description = newTaskDescriptionInput.value.trim();
    if (description === '') {
        alert('Task description cannot be empty.'); // Use a simple alert for task-specific input validation
        return;
    }

    try {
        const response = await fetch('/tasks', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ description, status: 'todo' }) // Default status
        });
        const data = await response.json(); // Expect JSON response

        if (response.ok) {
            newTaskDescriptionInput.value = ''; // Clear input
            fetchTasks(); // Reload tasks
            // No specific message for add task, success is implied by list refresh
        } else if (response.status === 401) {
            updateUIForAuth(false);
            showAuthMessage(data.message || 'Authentication required to add task. Please log in.', true);
        } else {
            alert('Failed to add task: ' + (data.message || data.error || 'Unknown error.'));
            console.error('Failed to add task:', response.status, data);
        }
    } catch (error) {
        console.error('Error adding task:', error);
        alert('Network error adding task.');
    }
});

// --- Modal Logic ---
function openTaskModal(task) {
    currentTaskId = task.id;
    modalTaskId.textContent = task.id;
    modalTaskDescriptionInput.value = task.description;
    modalTaskStatusSelect.value = task.status; // Set the selected option
    taskModal.style.display = 'flex'; // Show the modal
}

function closeTaskModal() {
    taskModal.style.display = 'none'; // Hide the modal
    currentTaskId = null; // Clear current task ID
}

closeButton.addEventListener('click', closeTaskModal);

// Close modal if user clicks outside of it
window.addEventListener('click', (event) => {
    if (event.target === taskModal) {
        closeTaskModal();
    }
});

// Update Task in Modal
updateTaskBtn.addEventListener('click', async () => {
    if (!currentTaskId) return;

    const newDescription = modalTaskDescriptionInput.value.trim();
    const newStatus = modalTaskStatusSelect.value;

    if (!newDescription) {
        alert('Description cannot be empty!');
        return;
    }

    try {
        const response = await fetch(`/tasks/${currentTaskId}`, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ description: newDescription, status: newStatus })
        });
        const data = await response.json(); // Expect JSON response

        if (response.ok) {
            closeTaskModal();
            fetchTasks(); // Reload tasks to show updated data
            // No specific message for update, success is implied by list refresh
        } else if (response.status === 401) {
            updateUIForAuth(false);
            showAuthMessage(data.message || 'Authentication required to update task. Please log in.', true);
            closeTaskModal(); // Close modal on auth error
        } else {
            alert('Failed to update task: ' + (data.message || data.error || 'Unknown error.'));
            console.error('Failed to update task:', response.status, data);
        }
    } catch (error) {
        console.error('Error updating task:', error);
        alert('Network error updating task.');
    }
});

// Delete Task in Modal
deleteTaskBtn.addEventListener('click', async () => {
    if (!currentTaskId) return;

    if (!confirm(`Are you sure you want to delete task ID: ${currentTaskId}?`)) {
        return; // User cancelled
    }

    try {
        const response = await fetch(`/tasks/${currentTaskId}`, {
            method: 'DELETE'
        });

        // For DELETE, 204 No Content is a common successful response, no JSON body
        if (response.status === 204) {
            closeTaskModal();
            fetchTasks(); // Reload tasks to reflect deletion
            // No specific message for delete, success is implied by list refresh
        } else if (response.status === 401) {
            const data = await response.json(); // Attempt to read JSON error
            updateUIForAuth(false);
            showAuthMessage(data.message || 'Authentication required to delete task. Please log in.', true);
            closeTaskModal(); // Close modal on auth error
        } else {
            const data = await response.json(); // Still try to parse JSON for other errors
            alert('Failed to delete task: ' + (data.message || data.error || 'Unknown error.'));
            console.error('Failed to delete task:', response.status, data);
        }
    } catch (error) {
        console.error('Error deleting task:', error);
        alert('Network error deleting task.');
    }
});