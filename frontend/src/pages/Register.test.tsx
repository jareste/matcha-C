import { screen, within } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { afterEach, beforeEach, describe, expect, test, vi } from 'vitest'
import { renderAt } from '../test/utils.tsx'

const fetchMock = vi.fn()

function jsonResponse(status: number, body: unknown) {
  return new Response(JSON.stringify(body), { status, headers: { 'Content-Type': 'application/json' } })
}

async function fillForm(overrides: Partial<Record<string, string>> = {}) {
  const values = {
    Username: 'alice',
    Email: 'alice@example.com',
    'First name': 'Alice',
    'Last name': 'Liddell',
    Password: 'Wonder1and',
    'Confirm password': 'Wonder1and',
    ...overrides,
  }
  for (const [label, value] of Object.entries(values)) {
    if (value) await userEvent.type(screen.getByLabelText(label), value)
  }
}

function submit() {
  return userEvent.click(screen.getByRole('button', { name: 'Create account' }))
}

beforeEach(() => {
  fetchMock.mockReset()
  vi.stubGlobal('fetch', fetchMock)
})

afterEach(() => {
  vi.unstubAllGlobals()
})

describe('register form', () => {
  test('posts valid input to /api/register and goes to the check-email page', async () => {
    fetchMock.mockResolvedValue(jsonResponse(201, { success: true, message: 'Registration successful' }))
    renderAt('/register')

    await fillForm()
    await submit()

    expect(await screen.findByRole('heading', { level: 1, name: 'Check your email' })).toBeInTheDocument()
    expect(screen.getByText('alice@example.com')).toBeInTheDocument()

    expect(fetchMock).toHaveBeenCalledTimes(1)
    const [url, init] = fetchMock.mock.calls[0]
    expect(url).toMatch(/\/api\/register$/)
    expect(JSON.parse(init.body)).toEqual({
      username: 'alice',
      email: 'alice@example.com',
      first_name: 'Alice',
      last_name: 'Liddell',
      password: 'Wonder1and',
    })
  })

  test('shows required-field errors and does not call the server', async () => {
    renderAt('/register')
    await submit()

    expect(screen.getByText('Username is required')).toBeInTheDocument()
    expect(screen.getByText('Email is required')).toBeInTheDocument()
    expect(screen.getByText('First name is required')).toBeInTheDocument()
    expect(screen.getByText('Last name is required')).toBeInTheDocument()
    expect(screen.getByText('Password is required')).toBeInTheDocument()
    expect(screen.getByLabelText('Username')).toHaveAttribute('aria-invalid', 'true')
    expect(fetchMock).not.toHaveBeenCalled()
  })

  test('rejects a weak password and keeps what the user typed', async () => {
    renderAt('/register')
    await fillForm({ Password: 'weak', 'Confirm password': 'weak' })
    await submit()

    expect(screen.getByText(/^Password needs:/)).toBeInTheDocument()
    expect(fetchMock).not.toHaveBeenCalled()
    expect(screen.getByLabelText('Username')).toHaveValue('alice')
    expect(screen.getByLabelText('Email')).toHaveValue('alice@example.com')
  })

  test('rejects a mismatched confirmation', async () => {
    renderAt('/register')
    await fillForm({ 'Confirm password': 'Wonder1ane' })
    await submit()

    expect(screen.getByText('Passwords do not match')).toBeInTheDocument()
    expect(fetchMock).not.toHaveBeenCalled()
  })

  test('password rules tick as the user types', async () => {
    renderAt('/register')
    const rules = screen.getByRole('list', { name: 'Password rules' })
    const rule = (name: RegExp) => within(rules).getByText(name)

    expect(rule(/At least 8 characters/)).toHaveAttribute('data-met', 'false')
    await userEvent.type(screen.getByLabelText('Password'), 'abcdefgh')
    expect(rule(/At least 8 characters/)).toHaveAttribute('data-met', 'true')
    expect(rule(/A lowercase letter/)).toHaveAttribute('data-met', 'true')
    expect(rule(/An uppercase letter/)).toHaveAttribute('data-met', 'false')
  })

  test('shows a backend error next to the matching field and keeps the input', async () => {
    fetchMock.mockResolvedValue(jsonResponse(400, { error: 'Email already exists' }))
    renderAt('/register')
    await fillForm()
    await submit()

    const email = screen.getByLabelText('Email')
    expect(await screen.findByText('Email already exists')).toHaveAttribute('id', 'email-error')
    expect(email).toHaveAttribute('aria-invalid', 'true')
    expect(email).toHaveValue('alice@example.com')
    expect(screen.getByLabelText('Username')).toHaveValue('alice')
    expect(screen.getByRole('heading', { level: 1, name: 'Sign up' })).toBeInTheDocument()
  })

  test('editing a field clears its error', async () => {
    fetchMock.mockResolvedValue(jsonResponse(400, { error: 'Username already exists' }))
    renderAt('/register')
    await fillForm()
    await submit()

    expect(await screen.findByText('Username already exists')).toBeInTheDocument()
    await userEvent.type(screen.getByLabelText('Username'), '2')
    expect(screen.queryByText('Username already exists')).not.toBeInTheDocument()
  })

  test('shows non-field backend errors at the top of the form', async () => {
    fetchMock.mockResolvedValue(jsonResponse(500, { error: 'Failed to register user' }))
    renderAt('/register')
    await fillForm()
    await submit()

    expect(await screen.findByRole('alert')).toHaveTextContent('Failed to register user')
  })

  test('reports a network failure', async () => {
    fetchMock.mockRejectedValue(new TypeError('Failed to fetch'))
    renderAt('/register')
    await fillForm()
    await submit()

    expect(await screen.findByRole('alert')).toHaveTextContent('Cannot reach the server')
  })

  test('never logs or stores the password', async () => {
    const logSpies = (['log', 'info', 'debug', 'warn', 'error'] as const).map((m) => vi.spyOn(console, m))
    fetchMock.mockResolvedValue(jsonResponse(201, { success: true }))
    renderAt('/register')
    await fillForm()
    await submit()
    await screen.findByRole('heading', { level: 1, name: 'Check your email' })

    for (const spy of logSpies) {
      for (const args of spy.mock.calls) expect(JSON.stringify(args)).not.toContain('Wonder1and')
      spy.mockRestore()
    }
    const stored = JSON.stringify({ ...localStorage }) + JSON.stringify({ ...sessionStorage })
    expect(stored).not.toContain('Wonder1and')
    expect(JSON.stringify(window.history.state)).not.toContain('Wonder1and')
  })
})
