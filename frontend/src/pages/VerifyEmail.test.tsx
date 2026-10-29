import { render, screen, within } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { StrictMode } from 'react'
import { MemoryRouter } from 'react-router'
import { afterEach, beforeEach, describe, expect, test, vi } from 'vitest'
import App from '../App.tsx'
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

describe('verify email page', () => {
  test('submits the token from the link and confirms the account', async () => {
    fetchMock.mockResolvedValue(jsonResponse(200, { success: true, message: 'Email verified' }))
    renderAt('/verify-email?token=abc123')

    expect(screen.getByRole('heading', { level: 1, name: 'Verifying your email…' })).toBeInTheDocument()
    expect(await heading('Email verified')).toBeInTheDocument()
    expect(within(screen.getByRole('main')).getByRole('link', { name: 'Log in' })).toHaveAttribute('href', '/login')

    expect(fetchMock).toHaveBeenCalledTimes(1)
    const [url, init] = fetchMock.mock.calls[0]
    expect(url).toMatch(/\/api\/verify-email$/)
    expect(JSON.parse(init.body)).toEqual({ token: 'abc123' })
  })

  test('shows a recoverable message for an expired link', async () => {
    fetchMock.mockResolvedValue(jsonResponse(410, { error: 'Token expired' }))
    renderAt('/verify-email?token=old')

    expect(await heading('Link expired')).toBeInTheDocument()
    expect(screen.getByRole('alert')).toHaveTextContent('expired')
    expect(screen.getByRole('link', { name: 'Sign up again' })).toHaveAttribute('href', '/register')
    expect(within(screen.getByRole('main')).getByRole('link', { name: 'Log in' })).toHaveAttribute('href', '/login')
  })

  test('shows a recoverable message for an invalid link', async () => {
    fetchMock.mockResolvedValue(jsonResponse(400, { error: 'Invalid token' }))
    renderAt('/verify-email?token=bogus')

    expect(await heading('Invalid link')).toBeInTheDocument()
    expect(screen.getByRole('link', { name: 'Sign up again' })).toBeInTheDocument()
    expect(screen.queryByText('Email verified')).not.toBeInTheDocument()
  })

  test('treats a link without token as invalid without calling the server', async () => {
    renderAt('/verify-email')

    expect(await heading('Invalid link')).toBeInTheDocument()
    expect(fetchMock).not.toHaveBeenCalled()
  })

  test('does not claim success when the server fails, and lets the user retry', async () => {
    fetchMock.mockResolvedValueOnce(jsonResponse(500, { error: 'Failed to verify email' }))
    fetchMock.mockResolvedValueOnce(jsonResponse(200, { success: true }))
    renderAt('/verify-email?token=abc123')

    expect(await heading('Verification failed')).toBeInTheDocument()
    expect(screen.getByRole('alert')).toHaveTextContent('Failed to verify email')

    await userEvent.click(screen.getByRole('button', { name: 'Try again' }))
    expect(await heading('Email verified')).toBeInTheDocument()
    expect(fetchMock).toHaveBeenCalledTimes(2)
  })

  test('reports a network failure', async () => {
    fetchMock.mockRejectedValue(new TypeError('Failed to fetch'))
    renderAt('/verify-email?token=abc123')

    expect(await heading('Verification failed')).toBeInTheDocument()
    expect(screen.getByRole('alert')).toHaveTextContent('Cannot reach the server')
  })

  test('submits the single-use token only once under StrictMode', async () => {
    fetchMock.mockResolvedValue(jsonResponse(200, { success: true }))
    render(
      <StrictMode>
        <MemoryRouter initialEntries={['/verify-email?token=abc123']}>
          <App />
        </MemoryRouter>
      </StrictMode>,
    )

    expect(await heading('Email verified')).toBeInTheDocument()
    expect(fetchMock).toHaveBeenCalledTimes(1)
  })
})
