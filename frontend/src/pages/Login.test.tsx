import { screen, within } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { beforeEach, describe, expect, test, vi } from 'vitest'
import { jsonResponse, renderAt, session, stubApi } from '../test/utils.tsx'

const fetchMock = vi.fn()

// Mimics the backend: a successful login opens the session read by /api/validate.
function acceptLogin() {
  fetchMock.mockImplementation(async (url: string) => {
    if (url.endsWith('/api/login')) {
      session.username = 'alice'
      return jsonResponse(200, { success: true, message: 'Login successful' })
    }
    if (url.endsWith('/api/logout')) {
      session.username = null
      return new Response(null, { status: 200 })
    }
    throw new Error(`unexpected call to ${url}`)
  })
}

async function fillForm(email = 'alice@example.com', password = 'Wonder1and') {
  if (email) await userEvent.type(screen.getByLabelText('Email'), email)
  if (password) await userEvent.type(screen.getByLabelText('Password'), password)
}

function submit() {
  return userEvent.click(screen.getByRole('button', { name: 'Log in' }))
}

function header() {
  return screen.getByRole('banner')
}

beforeEach(() => {
  fetchMock.mockReset()
  stubApi(fetchMock)
})

describe('login', () => {
  test('valid credentials open a session and lead into the app', async () => {
    acceptLogin()
    renderAt('/login')
    await fillForm()
    await submit()

    expect(await screen.findByRole('heading', { level: 1, name: 'Home' })).toBeInTheDocument()
    expect(screen.getByText(/Welcome, alice/)).toBeInTheDocument()
    expect(within(header()).getByRole('button', { name: 'Log out' })).toBeInTheDocument()

    expect(fetchMock).toHaveBeenCalledTimes(1)
    const [url, init] = fetchMock.mock.calls[0]
    expect(url).toMatch(/\/api\/login$/)
    expect(init).toMatchObject({ method: 'POST', credentials: 'include' })
    expect(JSON.parse(init.body)).toEqual({ email: 'alice@example.com', password: 'Wonder1and' })
  })

  test('returns to the private page that was requested before logging in', async () => {
    acceptLogin()
    renderAt('/')
    await screen.findByRole('heading', { level: 1, name: 'Log in' })
    await fillForm()
    await submit()

    expect(await screen.findByRole('heading', { level: 1, name: 'Home' })).toBeInTheDocument()
  })

  test('an already logged-in user is sent into the app', async () => {
    session.username = 'alice'
    renderAt('/login')
    expect(await screen.findByRole('heading', { level: 1, name: 'Home' })).toBeInTheDocument()
  })

  test.each(['Invalid credentials', 'Email not found'])('shows the backend error "%s"', async (error) => {
    fetchMock.mockResolvedValue(jsonResponse(400, { error }))
    renderAt('/login')
    await fillForm()
    await submit()

    expect(await screen.findByRole('alert')).toHaveTextContent(error)
    expect(screen.getByRole('heading', { level: 1, name: 'Log in' })).toBeInTheDocument()
    expect(screen.getByLabelText('Email')).toHaveValue('alice@example.com')
    expect(screen.getByRole('button', { name: 'Log in' })).toBeEnabled()
  })

  test('requires both fields without calling the server', async () => {
    renderAt('/login')
    await submit()

    expect(screen.getByText('Email is required')).toBeInTheDocument()
    expect(screen.getByText('Password is required')).toBeInTheDocument()
    expect(screen.getByLabelText('Email')).toHaveAttribute('aria-invalid', 'true')
    expect(fetchMock).not.toHaveBeenCalled()
  })

  test('reports a network failure', async () => {
    fetchMock.mockRejectedValue(new TypeError('Failed to fetch'))
    renderAt('/login')
    await fillForm()
    await submit()

    expect(await screen.findByRole('alert')).toHaveTextContent('Cannot reach the server')
  })

  test('does not claim success when the session cookie was not kept', async () => {
    // Login answers 200 but /api/validate still sees no cookie.
    fetchMock.mockResolvedValue(jsonResponse(200, { success: true }))
    renderAt('/login')
    await fillForm()
    await submit()

    expect(await screen.findByRole('alert')).toHaveTextContent('session could not be started')
    expect(screen.getByRole('heading', { level: 1, name: 'Log in' })).toBeInTheDocument()
  })

  test('never logs or stores the password or the session', async () => {
    const logSpies = (['log', 'info', 'debug', 'warn', 'error'] as const).map((m) => vi.spyOn(console, m))
    acceptLogin()
    renderAt('/login')
    await fillForm()
    await submit()
    await screen.findByRole('heading', { level: 1, name: 'Home' })

    for (const spy of logSpies) {
      for (const args of spy.mock.calls) expect(JSON.stringify(args)).not.toContain('Wonder1and')
      spy.mockRestore()
    }
    expect(localStorage.length).toBe(0)
    expect(sessionStorage.length).toBe(0)
    expect(JSON.stringify(window.history.state)).not.toContain('Wonder1and')
  })
})

describe('logout', () => {
  test('invalidates the session on the backend and returns to login', async () => {
    acceptLogin()
    session.username = 'alice'
    renderAt('/')
    await userEvent.click(await within(header()).findByRole('button', { name: 'Log out' }))

    expect(await screen.findByRole('heading', { level: 1, name: 'Log in' })).toBeInTheDocument()
    expect(within(header()).getByRole('link', { name: 'Log in' })).toBeInTheDocument()
    expect(within(header()).queryByRole('button', { name: 'Log out' })).not.toBeInTheDocument()

    const [url, init] = fetchMock.mock.calls[0]
    expect(url).toMatch(/\/api\/logout$/)
    expect(init).toMatchObject({ method: 'POST', credentials: 'include' })
  })

  test('stays logged in and says so when the backend cannot be reached', async () => {
    session.username = 'alice'
    fetchMock.mockRejectedValue(new TypeError('Failed to fetch'))
    renderAt('/')
    await userEvent.click(await within(header()).findByRole('button', { name: 'Log out' }))

    expect(await screen.findByRole('alert')).toHaveTextContent('Could not log out')
    expect(screen.getByRole('heading', { level: 1, name: 'Home' })).toBeInTheDocument()
    expect(within(header()).getByRole('button', { name: 'Log out' })).toBeEnabled()
  })
})
