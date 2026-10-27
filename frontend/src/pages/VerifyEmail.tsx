import { useCallback, useEffect, useRef, useState } from 'react'
import { Link, useSearchParams } from 'react-router'
import { verifyEmail, type VerifyOutcome } from '../lib/verifyEmail.ts'

const linkClass = 'font-medium text-brand hover:underline'
const buttonClass =
  'inline-block rounded-md bg-brand px-4 py-2 text-sm font-medium text-white hover:bg-brand-dark disabled:opacity-60'

function VerifyEmail() {
  const token = useSearchParams()[0].get('token')?.trim() ?? ''
  const [outcome, setOutcome] = useState<VerifyOutcome | null>(token ? null : { kind: 'invalid' })
  // The token is single-use: StrictMode's double effect must not submit it twice.
  const submitted = useRef('')

  const submit = useCallback(async () => {
    setOutcome(null)
    setOutcome(await verifyEmail(token))
  }, [token])

  useEffect(() => {
    if (!token || submitted.current === token) return
    submitted.current = token
    void submit()
  }, [token, submit])

  return (
    <section className="mx-auto w-full max-w-sm">
      {outcome === null && (
        <>
          <h1 className="text-2xl font-semibold sm:text-3xl">Verifying your email…</h1>
          <p role="status" className="mt-2 text-gray-600 dark:text-gray-400">
            Please wait while we confirm your account.
          </p>
        </>
      )}

      {outcome?.kind === 'success' && (
        <>
          <h1 className="text-2xl font-semibold sm:text-3xl">Email verified</h1>
          <p className="mt-2 text-gray-600 dark:text-gray-400">Your account is active. You can now log in.</p>
          <p className="mt-6">
            <Link to="/login" className={buttonClass}>
              Log in
            </Link>
          </p>
        </>
      )}

      {outcome?.kind === 'expired' && (
        <>
          <h1 className="text-2xl font-semibold sm:text-3xl">Link expired</h1>
          <p role="alert" className="mt-2 text-gray-600 dark:text-gray-400">
            This verification link has expired. Sign up again to receive a new one, or log in if your account is already
            active.
          </p>
          <RecoveryLinks />
        </>
      )}

      {outcome?.kind === 'invalid' && (
        <>
          <h1 className="text-2xl font-semibold sm:text-3xl">Invalid link</h1>
          <p role="alert" className="mt-2 text-gray-600 dark:text-gray-400">
            This verification link is invalid or has already been used. Check that you opened the full link from the
            email, or log in if your account is already active.
          </p>
          <RecoveryLinks />
        </>
      )}

      {outcome?.kind === 'error' && (
        <>
          <h1 className="text-2xl font-semibold sm:text-3xl">Verification failed</h1>
          <p role="alert" className="mt-2 text-gray-600 dark:text-gray-400">
            {outcome.message}
          </p>
          <p className="mt-6">
            <button type="button" onClick={() => void submit()} className={buttonClass}>
              Try again
            </button>
          </p>
        </>
      )}
    </section>
  )
}

function RecoveryLinks() {
  return (
    <p className="mt-6 flex gap-4 text-sm">
      <Link to="/register" className={linkClass}>
        Sign up again
      </Link>
      <Link to="/login" className={linkClass}>
        Log in
      </Link>
    </p>
  )
}

export default VerifyEmail
