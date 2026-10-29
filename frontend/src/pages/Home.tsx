import { useAuth } from '../lib/authContext.ts'

// Private page: only rendered under <RequireAuth />.
function Home() {
  const { user } = useAuth()

  return (
    <section>
      <h1 className="text-2xl font-semibold sm:text-3xl">Home</h1>
      <p className="mt-2 text-gray-600 dark:text-gray-400">
        Welcome{user ? `, ${user.username}` : ''}. Suggestions and matches will show up here.
      </p>
    </section>
  )
}

export default Home
