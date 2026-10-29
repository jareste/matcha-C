import { screen, within } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { afterEach, beforeEach, describe, expect, test, vi } from 'vitest'
import { jsonResponse, renderAt, stubApi } from '../test/utils.tsx'

const fetchMock = vi.fn()

function heading(name: string) {
  return screen.findByRole('heading', { level: 1, name })
}

beforeEach(() => {
  fetchMock.mockReset()
  stubApi(fetchMock)
})

afterEach(() => {
  vi.unstubAllGlobals()
})

describe('forgot password page', () => {
  test('is reachable from the login page', async () => {
    renderAt('/login')
    await userEvent.click(screen.getByRole('link', { name: 'Forgot your password?' }))
    expect(await heading('Forgot your password?')).toBeInTheDocument()
  })

  test('sends the email and shows a neutral confirmation', async () => {
    fetchMock.mockResolvedValue(jsonResponse(200, { success: true }))
    renderAt('/forgot-password')

    await userEvent.type(screen.getByLabelText('Email'), 'a@b.co')
    await userEvent.click(screen.getByRole('button', { name: 'Send reset link' }))

    expect(await heading('Check your email')).toBeInTheDocument()
    expect(screen.getByRole('status')).toHaveTextContent('If an account exists for a@b.co')
    const [url, init] = fetchMock.mock.calls[0]
    expect(url).toMatch(/\/api\/forgot-password$/)
    expect(JSON.parse(init.body)).toEqual({ email: 'a@b.co' })
  })

  test('shows the same confirmation when the backend says the email is unknown', async () => {
    fetchMock.mockResolvedValue(jsonResponse(400, { error: 'Email not found' }))
    renderAt('/forgot-password')

    await userEvent.type(screen.getByLabelText('Email'), 'nobody@b.co')
    await userEvent.click(screen.getByRole('button', { name: 'Send reset link' }))

    expect(await heading('Check your email')).toBeInTheDocument()
    expect(screen.queryByText(/not found/i)).not.toBeInTheDocument()
  })

  test('validates the email before calling the server', async () => {
    renderAt('/forgot-password')

    await userEvent.click(screen.getByRole('button', { name: 'Send reset link' }))
    expect(screen.getByText('Email is required')).toBeInTheDocument()

    await userEvent.type(screen.getByLabelText('Email'), 'nope')
    await userEvent.click(screen.getByRole('button', { name: 'Send reset link' }))
    expect(screen.getByText('Enter a valid email address')).toBeInTheDocument()
    expect(fetchMock).not.toHaveBeenCalled()
  })

  test('keeps the form and reports a server failure', async () => {
    fetchMock.mockResolvedValue(jsonResponse(500, { error: 'Failed to send email' }))
    renderAt('/forgot-password')

    await userEvent.type(screen.getByLabelText('Email'), 'a@b.co')
    await userEvent.click(screen.getByRole('button', { name: 'Send reset link' }))

    expect(await screen.findByRole('alert')).toHaveTextContent('Failed to send email')
    expect(screen.getByRole('button', { name: 'Send reset link' })).toBeInTheDocument()
  })
})

describe('reset password page', () => {
  async function fillAndSubmit(password: string, confirm = password) {
    await userEvent.type(screen.getByLabelText('New password'), password)
    if (confirm) await userEvent.type(screen.getByLabelText('Confirm new password'), confirm)
    await userEvent.click(screen.getByRole('button', { name: 'Update password' }))
  }

  test('submits the token and the new password, then links to log in', async () => {
    fetchMock.mockResolvedValue(jsonResponse(200, { success: true, message: 'Password updated' }))
    renderAt('/reset-password?token=abc123')

    expect(screen.getByRole('heading', { level: 1, name: 'Choose a new password' })).toBeInTheDocument()
    await fillAndSubmit('Strong1pass')

    expect(await heading('Password updated')).toBeInTheDocument()
    expect(within(screen.getByRole('main')).getByRole('link', { name: 'Log in' })).toHaveAttribute('href', '/login')
    const [url, init] = fetchMock.mock.calls[0]
    expect(url).toMatch(/\/api\/reset-password$/)
    expect(JSON.parse(init.body)).toEqual({ token: 'abc123', password: 'Strong1pass' })
  })

  test('rejects a weak or mismatched password without calling the server', async () => {
    renderAt('/reset-password?token=abc123')

    await fillAndSubmit('short', 'other')
    expect(screen.getByText(/Password needs:/)).toBeInTheDocument()
    expect(screen.getByText('Passwords do not match')).toBeInTheDocument()
    expect(fetchMock).not.toHaveBeenCalled()
  })

  test('shows a clear message for an expired token', async () => {
    fetchMock.mockResolvedValue(jsonResponse(410, { error: 'Token expired' }))
    renderAt('/reset-password?token=old')

    await fillAndSubmit('Strong1pass')
    expect(await heading('Link expired')).toBeInTheDocument()
    expect(screen.getByRole('alert')).toHaveTextContent('expired')
    expect(screen.getByRole('link', { name: 'Request a new link' })).toHaveAttribute('href', '/forgot-password')
  })

  test('shows a clear message for an invalid token', async () => {
    fetchMock.mockResolvedValue(jsonResponse(400, { error: 'Invalid token' }))
    renderAt('/reset-password?token=bogus')

    await fillAndSubmit('Strong1pass')
    expect(await heading('Invalid link')).toBeInTheDocument()
    expect(screen.getByRole('link', { name: 'Request a new link' })).toBeInTheDocument()
    expect(screen.queryByText('Password updated')).not.toBeInTheDocument()
  })

  test('treats a link without token as invalid without calling the server', async () => {
    renderAt('/reset-password')

    expect(await heading('Invalid link')).toBeInTheDocument()
    expect(fetchMock).not.toHaveBeenCalled()
  })

  test('shows a backend password rejection on the field and keeps the form', async () => {
    fetchMock.mockResolvedValue(jsonResponse(400, { error: 'Password too weak' }))
    renderAt('/reset-password?token=abc123')

    await fillAndSubmit('Strong1pass')
    expect(await screen.findByText('Password too weak')).toBeInTheDocument()
    expect(screen.getByLabelText('New password')).toHaveAttribute('aria-invalid', 'true')
  })

  test('does not claim success when the server fails, and lets the user retry', async () => {
    fetchMock.mockResolvedValueOnce(jsonResponse(500, { error: 'Failed to reset password' }))
    fetchMock.mockResolvedValueOnce(jsonResponse(200, { success: true }))
    renderAt('/reset-password?token=abc123')

    await fillAndSubmit('Strong1pass')
    expect(await screen.findByRole('alert')).toHaveTextContent('Failed to reset password')

    await userEvent.click(screen.getByRole('button', { name: 'Update password' }))
    expect(await heading('Password updated')).toBeInTheDocument()
    expect(fetchMock).toHaveBeenCalledTimes(2)
  })
})
